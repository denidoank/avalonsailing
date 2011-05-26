// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Communication daemon for sending/receiving status messages and commands by
// SMS.
// ./comm --logtostderr --no-syslog --task=comm --timeout=60 --debug \
//        --device=/dev/ttyUSB1 --phone=41760000000

#include "lib/fm/fm.h"
#include "lib/fm/log.h"
#include "modem/modem.h"

#include <getopt.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

using namespace std;

#define DEFAULT_MODEM_DEVICE      "/dev/ttyUSB1"

static char modem_device[256] = DEFAULT_MODEM_DEVICE;
static char phone_number[64] = "";

static void Init(int argc, char** argv) {
  static const struct option long_options[] = {
    { "device", 1, NULL, 0},
    { "phone", 1, NULL, 0},
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
      default:
        FM_LOG_INFO("usage: %s [-d modem dev] [-p phone number]\n"
                    "\t -device modem serial device (default %s)\n"
                    "\t -phone phone number used for sending SMS messages\n",
                    argv[0], DEFAULT_MODEM_DEVICE);
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

  Modem m;
  m.Init(modem_device);
  string modem_type, modem_sn;
  m.GetModemType(&modem_type);
  m.GetModemSerialNumber(&modem_sn);
  FM_LOG_INFO("Found modem: %s SN: %s\n", modem_type.c_str(), modem_sn.c_str());

  while (true) {  // Wait for asynchronous status changes.
    // TODO(marius): Read queued messages and send SMS.
    // Send SMS messages:
    // FM_LOG_INFO("Sending SMS: \"%s\" to phone number: %s ...\n",
    //        message, phone_number);
    // if (m.SendSMSMessage(phone_number, message) != Modem::OK) {
    //   FM_LOG_ERROR("SMS sending error!\n");
    // } else {
    //   FM_LOG_INFO("SMS sending successfull.\n");
    // }

    // Check for new SMS messages.
    list<Modem::SmsMessage> messages;
    m.ReceiveSMSMessages(&messages);
    while (!messages.empty()) {
      FM_LOG_INFO("Received SMS Message time=%d, phone=%s text=\"%s\"\n",
                  (int)messages.front().time,
                  messages.front().phone_number.c_str(),
                  messages.front().text.c_str());
       messages.pop_front();
    }
    m.IdleLoop();

    sleep(10);

    // Check that modem is still alive by issuing a simple command.
    string modem_sn2;
    if (m.GetModemSerialNumber(&modem_sn2) != Modem::OK ||
        modem_sn2 != modem_sn) {
      FM_LOG_ERROR("Modem not responsive. Get modem serial number failed!");
      return 1;  // TODO(marius): Maybe retry few times.
    }
    FM::Keepalive();
  }
}
