// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Communication daemon for sending/receiving status messages and commands by
// SMS.
// ./modemd --logtostderr --no-syslog --task=modemd --timeout=60 --debug \
//          --device=/dev/ttyUSB1 --phone=41760000000

#include "lib/fm/fm.h"
#include "lib/fm/log.h"
#include "modem/message-queue.h"
#include "modem/modem.h"

#include <getopt.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unistd.h>

using namespace std;

#define DEFAULT_MODEM_DEVICE      "/dev/ttyUSB1"
#define DEFAULT_QUEUE             "/tmp"

static char modem_device[256] = DEFAULT_MODEM_DEVICE;
static char phone_number[64] = "";
static char queue[256] = DEFAULT_QUEUE;

static void Init(int argc, char** argv) {
  static const struct option long_options[] = {
    { "device", 1, NULL, 0},
    { "phone", 1, NULL, 0},
    { "queue", 1, NULL, 0},
    { "help", 0, NULL, 0},
    { NULL, 0, NULL, 0 }
  };
  
  optind = 1;
  opterr = 0;
  int opt_index;
  int opt;
  while ((opt = getopt_long(argc, argv, "", long_options, &opt_index)) != -1) {
    if (opt != 0)  // Invalid argument, probably used by other module.
      continue;
    switch(opt_index) {
      case 0:  // --device=...
        strncpy(modem_device, optarg, sizeof(modem_device));
        break;
      case 1:  // --phone=...
        strncpy(phone_number, optarg, sizeof(phone_number));
        break;
      case 2:  // --queue=...
        strncpy(phone_number, optarg, sizeof(queue));
        break;
      default:
        FM_LOG_INFO("usage: %s [--device dev] [--queue dir] [--phone no]\n"
                    "\t --device modem serial device (default %s)\n"
                    "\t --phone phone number used for sending SMS messages\n",
                    "\t --queue directory queue for messages (default %s)\n",
                    argv[0], DEFAULT_MODEM_DEVICE, DEFAULT_QUEUE);
        exit(0);
    }
  }
}

int main(int argc, char** argv) {
  FM::Init(argc, argv);
  Init(argc, argv);

  FM_LOG_INFO("Using modem device: %s and phone number:\n",
              modem_device, phone_number);
  if (strlen(phone_number) == 0) {
    FM_LOG_FATAL("No phone number specified for status messages.");
  }

  char inbox_dir[256];
  strcpy(inbox_dir, queue);
  strcat(inbox_dir, "/modem-inbox");
  char outbox_dir[256];
  strcpy(outbox_dir, queue);
  strcat(outbox_dir, "/modem-outbox");
  MessageQueue inbox(inbox_dir);
  MessageQueue outbox(outbox_dir);

  Modem m;
  m.Init(modem_device);
  string modem_type, modem_sn;
  m.GetModemType(&modem_type);
  m.GetModemSerialNumber(&modem_sn);
  FM_LOG_INFO("Found modem: %s SN: %s\n", modem_type.c_str(), modem_sn.c_str());

  while (true) {  // Wait for asynchronous status changes.

    // Send outbox messages.
    while (outbox.NumMessages() > 0) {
      // Send SMS messages:
      string message;
      const MessageQueue::MessageId id = outbox.GetMessage(0, &message);
      if (id != MessageQueue::kInvalidId) {  // Read successfull.
        FM_LOG_INFO("Sending SMS: \"%s\" to phone number: %s ...\n",
                    message.c_str(), phone_number);
	Modem::ResultCode rc = m.SendSMSMessage(phone_number, message);
        if (rc != Modem::OK) {
          FM_LOG_ERROR("SMS sending error (%d)!\n", rc);
        } else {
          FM_LOG_INFO("SMS sending successfull.\n");
          outbox.DeleteMessage(id);  // Message sent. Delete it.
        }
      }
    }

    // Check for new SMS messages.
    list<Modem::SmsMessage> messages;
    m.ReceiveSMSMessages(&messages);
    while (!messages.empty()) {
      FM_LOG_INFO("Received SMS Message time=%d, phone=%s text=\"%s\"\n",
                  static_cast<int>(messages.front().time),
                  messages.front().phone_number.c_str(),
                  messages.front().text.c_str());
      const MessageQueue::MessageId id =
          inbox.PushMessage(messages.front().text);
      if (id != MessageQueue::kInvalidId) {
        m.DeleteSMSMessage(messages.front());
      }
      messages.pop_front();        
    }

    // Idle loop.
    m.IdleLoop();
    sleep(1);

    // Check that modem is still alive by issuing a simple command.
    int retries = 10;
    while (retries) {
      string modem_sn2;
      if (m.GetModemSerialNumber(&modem_sn2) != Modem::OK ||
          modem_sn2 != modem_sn) {
        FM_LOG_ERROR("Modem not responsive. Get modem serial number failed!");
        retries--;
        sleep(3);
      } else {
        break;
      }
    }
    if (retries == 0) {
      FM_LOG_ERROR("Modem not responsive. "
                   "Get modem serial number failed several times!");
      return -1;
    } else {
      FM::Keepalive();
    }

    // Check network registration and signal quality.
    bool registered = false;
    int signal_quality = 0;
    if (m.GetNetworkRegistration(&registered) == Modem::OK) {
      FM_LOG_INFO("Network registration: %s",
                  registered ? "registered" : "not-registered");
    }
    if (m.GetSignalQuality(&signal_quality) == Modem::OK) {
      FM_LOG_INFO("Signal quality: %d", signal_quality);
    }
    double lat = 0.0, lng = 0.0;
    time_t timestamp = 0;
    if (m.GetGeolocation(&lat, &lng, &timestamp) == Modem::OK) {
      FM_LOG_INFO("Coarse geolocation: (%5.3f X %5.3f), timestamp: %s",
                  lat, lng, ctime(&timestamp));
    }
  }
}
