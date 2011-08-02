// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Status daemon for regurarly compiling status messages and sending them
// over sms.
//
// ./statusd --logtostderr --no-syslog --task=statusd --timeout=60 --debug
//           --queue=/tmp

#include "lib/fm/fm.h"
#include "lib/fm/log.h"
#include "modem/message-queue.h"
#include "modem/status.h"
#include "proto/imu.h"
#include "proto/modem.h"
#include "proto/wind.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

using namespace std;

#define DEFAULT_QUEUE             "/tmp"
#define DEFAULT_INITIAL_TIMEOUT_S 10          // Wait up to 10s initially.
#define DEFAULT_STATUS_INTERVAL_S (6 * 3600)  // Send status messages every 6h.

static char queue[256] = DEFAULT_QUEUE;
static int initial_timeout = DEFAULT_INITIAL_TIMEOUT_S;
static int status_interval = DEFAULT_STATUS_INTERVAL_S;

static void Init(int argc, char** argv) {
  static const struct option long_options[] = {
    { "queue", 1, NULL, 0},
    { "initial_timeout", 1, NULL, 0},
    { "status_interval", 1, NULL, 0},
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
      case 0:  // --queue=...
        strncpy(queue, optarg, sizeof(queue));
        break;
      case 1:  // --initial_timeout=...
        initial_timeout = strtol(optarg, NULL, 10);
        break;
      case 2:  // --status_interval=...
        status_interval = strtol(optarg, NULL, 10);
        break;
      default:
        FM_LOG_INFO("usage: %s [--queue dir]\n"
                    "\t --queue directory queue for messages (default %s)\n",
                    argv[0], DEFAULT_QUEUE);
        exit(0);
    }
  }
}

int main(int argc, char** argv) {
  FM::Init(argc, argv);
  Init(argc, argv);

  FM_LOG_INFO("Using queue: %s, send status every %d seconds.",
              queue, status_interval);

  char outbox_dir[256];
  strcpy(outbox_dir, queue);
  strcat(outbox_dir, "/modem-outbox");
  MessageQueue outbox(outbox_dir);

  IMUProto imu_status = INIT_IMUPROTO;
  int imu_counter = 0;
  WindProto wind_status = INIT_WINDPROTO;
  int wind_counter = 0;
  ModemProto modem_status = INIT_MODEMPROTO;
  int modem_counter = 0;

  time_t start_time = time(NULL);
  time_t status_time = start_time - status_interval + initial_timeout;
  int status_counter = 0;

  while (true) {
    // Wait 10s for something to read on stdin.
    fd_set read_fds, write_fds, except_fds;
    FD_ZERO(&read_fds);
    FD_SET(fileno(stdin), &read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    if (select(fileno(stdin) + 1, &read_fds, &write_fds, &except_fds,
               &timeout) > 0) {  // No timeout, something to read available.
      char in[4096];
      fgets(in, sizeof(in), stdin);
      if (strlen(in) > 0 && in[strlen(in) - 1] == '\n') {
        in[strlen(in) - 1] = '\0';
      }

      {  // Check if this is a IMU status.
        IMUProto status;
        unsigned int n = 0;
        if (sscanf(in, IFMT_IMUPROTO(&status, &n)) > 0 && n == strlen(in)) {
          imu_status = status;
          imu_counter++;
        }
      }
      {  // Check if this is a wind sensor status.
        WindProto status;
        unsigned int n = 0;
        if (sscanf(in, IFMT_WINDPROTO(&status, &n)) > 0 && n == strlen(in)) {
          wind_status = status;
          wind_counter++;
        }
      }
      {  // Check if this is a modem status.
        ModemProto status;
        unsigned int n = 0;
        if (sscanf(in, IFMT_MODEMPROTO(&status, &n)) > 0 && n == strlen(in)) {
          modem_status = status;
          modem_counter++;
        }
      }

      if ((imu_counter + wind_counter + modem_counter) % 100) {
        FM_LOG_INFO("So far we got %d IMU reports, %d Wind reports, "
                    "%d Modem reports", imu_counter, wind_counter,
                    modem_counter);
      }
    } else {
      FM_LOG_ERROR("No status message received at stdin");
    }

    time_t now = time(NULL);
    if (now - status_time >= status_interval) {
      imu_counter = 0;
      wind_counter = 0;
      modem_counter = 0;

      // Send status message.
      string status_message =
	BuildStatusMessage(now, imu_status, wind_status, modem_status,
                           "Avalon boat status ok");
      FM_LOG_INFO("Status message: \"%s\"", status_message.c_str());
      outbox.PushMessage(status_message);
      
      int retries = 10;
      while (outbox.NumMessages() > 0 && retries > 0) {
        // Wait for successfully sending the SMS status message.
        FM_LOG_INFO("Waiting for status message to be sent.");
        sleep(1);
        retries--;
      }
      if (retries > 0) {
        FM_LOG_INFO("Status message sent.");
      } else {
        FM_LOG_ERROR("Status message still pending! Is modem deamon running?");
      }
      
      status_time = now;
      status_counter++;
    }

    FM::Keepalive();
  }
}
