// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Communication daemon for sending/receiving status messages and commands by
// SMS.
//   ./modemd --timeout=60 --device=/dev/ttyUSB1 --phone=41760000000

#include "modem/message-queue.h"
#include "modem/modem.h"
#include "proto/modem.h"

#include <getopt.h>
#include <syslog.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <time.h>
#include <unistd.h>

using namespace std;

#define DEFAULT_PHONE_NUMBER      "41763038610"
#define DEFAULT_MODEM_DEVICE      "/dev/ttyUSB8"
#define DEFAULT_QUEUE             "/tmp/modem"

static char modem_device[256] = DEFAULT_MODEM_DEVICE;
static char phone_number[64] = DEFAULT_PHONE_NUMBER;
static char queue[256] = DEFAULT_QUEUE;

static void init(int argc, char** argv) {
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
        strncpy(queue, optarg, sizeof(queue));
        break;
      default:
        printf("usage: %s [--device dev] [--queue dir] [--phone no]\n"
               "\t --device modem serial device (default %s)\n"
               "\t --phone phone number used for sending SMS messages\n"
               "\t --queue directory queue for messages (default %s)\n",
               argv[0], DEFAULT_MODEM_DEVICE, DEFAULT_QUEUE);
        exit(0);
    }
  }
}

int main(int argc, char** argv) {
  init(argc, argv);
  openlog(argv[0], LOG_PERROR, LOG_DAEMON);
  setlinebuf(stdout);
  setlinebuf(stderr);

  syslog(LOG_INFO, "Using modem device: %s, phone number: %s, queue: %s\n",
         modem_device, phone_number, queue);
  if (strlen(phone_number) == 0) {
    syslog(LOG_CRIT, "No phone number specified for status messages.");
    exit(1);
  }

  char inbox_dir[256];
  strcpy(inbox_dir, queue);
  strcat(inbox_dir, "/modem-inbox");
  char outbox_dir[256];
  strcpy(outbox_dir, queue);
  strcat(outbox_dir, "/modem-outbox");
  MessageQueue inbox(inbox_dir);
  MessageQueue outbox(outbox_dir);

  ModemProto status = INIT_MODEMPROTO;

  while (true) {  // Wait for asynchronous status changes.
    Modem m;
    if (!m.Init(modem_device)) {
      syslog(LOG_ERR, "Cannot initialize the modem: %s", modem_device);
      break;
    }
    string modem_type, modem_sn;

    // Check that modem is alive by issuing a simple command.
    int retries = 10;
    while (retries) {
      if (m.GetModemType(&modem_type) != Modem::OK ||
          m.GetModemSerialNumber(&modem_sn) != Modem::OK) {
        syslog(LOG_ERR, "Modem not responsive. Get serial number failed!");
        retries--;
        sleep(3);
      } else {
        break;
      }
    }
    if (retries == 0) {
      syslog(LOG_ERR, "Modem not responsive. "
                      "Get modem serial number failed several times!");
      break;
    }
    syslog(LOG_INFO, "Found modem: %s SN: %s",
           modem_type.c_str(), modem_sn.c_str());

    // Send outbox messages.
    retries = 10;
    syslog(LOG_INFO, "Messages in the outbox queue: %d", outbox.NumMessages());
    while (outbox.NumMessages() && retries) {
      // Send SMS messages:
      string message;
      const MessageQueue::MessageId id = outbox.GetMessage(0, &message);
      if (id != MessageQueue::kInvalidId) {  // Read successfull.
        syslog(LOG_INFO, "Sending SMS: \"%s\" to phone number: %s ...\n",
               message.c_str(), phone_number);
	Modem::ResultCode rc = m.SendSMSMessage(phone_number, message);
        if (rc != Modem::OK) {
          syslog(LOG_ERR, "SMS sending error (%d)!\n", rc);
          retries--;
        } else {
          syslog(LOG_INFO, "SMS sending successfull.\n");
          outbox.DeleteMessage(id);  // Message sent. Delete it.
        }
      }
    }
    if (retries == 0) {
      syslog(LOG_ERR, "Failed to send SMS messages. Check network or signal!");
    }

    // Check for new SMS messages.
    list<Modem::SmsMessage> messages;
    if (m.ReceiveSMSMessages(&messages) == Modem::OK) {
      syslog(LOG_INFO, "Received SMS messages: %d messages.", messages.size());
    } else {
      syslog(LOG_ERR, "Receive SMS messages: command failed!");
    }
    while (!messages.empty()) {
      syslog(LOG_INFO, "Received SMS Message time=%d, phone=%s text=\"%s\"\n",
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

    // Check network registration and signal quality.
    bool network_registration = false;
    if (m.GetNetworkRegistration(&network_registration) != Modem::OK) {
      syslog(LOG_ERR, "Checking network registration failed!");
    }
    status.network_registration = network_registration;
    if (m.GetSignalQuality(&status.signal_quality) != Modem::OK) {
      syslog(LOG_ERR, "Checking signal quality failed!");
    }
    if (m.GetGeolocation(&status.coarse_position_lat,
                         &status.coarse_position_lng,
                         &status.position_timestamp_s) != Modem::OK) {
      syslog(LOG_ERR, "Error retrieving geolocation!");
    }

    status.inbox_queue_messages = inbox.NumMessages();
    status.outbox_queue_messages = outbox.NumMessages();
    status.timestamp_s = time(NULL);

    int n = 0;
    char out[1024];
    snprintf(out, sizeof(out), OFMT_MODEMPROTO(status, &n));
    if (n > static_cast<int>(sizeof(out))) break;
    puts(out);

    // Idle loop.
    m.IdleLoop();
    sleep(1);
  }
  syslog(LOG_INFO, "Modem daemon terminated.");
}
