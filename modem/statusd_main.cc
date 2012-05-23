// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Status daemon for regurarly compiling status messages and sending them
// over sms.
//
// ./statusd --queue=/tmp --status_interval=10000 --remote_cmd_interval=5

#include "modem/message-queue.h"
#include "modem/status.h"
#include "proto/fuelcell.h"
#include "proto/helmsman.h"
#include "proto/helmsman_status.h"
#include "proto/imu.h"
#include "proto/modem.h"
#include "proto/remote.h"
#include "proto/wind.h"

#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/select.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

using namespace std;

#define DEFAULT_QUEUE             "/tmp/modem"
#define DEFAULT_INITIAL_TIMEOUT_S 180         // Wait up to 3min initially.
#define DEFAULT_STATUS_INTERVAL_S (24 * 3600) // Send status messages every 24h.
#define DEFAULT_REMOTE_CMD_INTERVAL_S 3       // Send command updates every 3s.
#define DEFAULT_STATUS_MESSAGE    "Avalon boat status ok"

static char queue[256] = DEFAULT_QUEUE;
static int initial_timeout = DEFAULT_INITIAL_TIMEOUT_S;
static int status_interval = DEFAULT_STATUS_INTERVAL_S;
static int remote_cmd_interval = DEFAULT_REMOTE_CMD_INTERVAL_S;
static char default_status[] = DEFAULT_STATUS_MESSAGE;
static bool quit = false;

static void signal_handler(int signo) {
  if (signo == SIGHUP || signo == SIGINT || signo == SIGQUIT || signo==SIGTERM)
    quit = true;
}

static void init(int argc, char** argv) {
  static const struct option long_options[] = {
    { "queue", 1, NULL, 0},
    { "initial_timeout", 1, NULL, 0},
    { "status_interval", 1, NULL, 0},
    { "remote_cmd_interval", 1, NULL, 0},
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
      case 3:  // --remote_cmd_interval=...
        remote_cmd_interval = strtol(optarg, NULL, 10);
        break;
      default:
        printf("usage: %s [--queue dir]\n"
               "\t --queue directory queue for messages (default %s)\n",
               argv[0], DEFAULT_QUEUE);
        exit(0);
    }
  }
}

int main(int argc, char** argv) {
  init(argc, argv);
  openlog(argv[0], LOG_PERROR, LOG_DAEMON);
  setlinebuf(stdout);
  setlinebuf(stderr);
  // Register signal handlers for a nice shutdown.
  signal(SIGHUP, &signal_handler);
  signal(SIGINT, &signal_handler);
  signal(SIGQUIT, &signal_handler);
  signal(SIGTERM, &signal_handler);

  syslog(LOG_INFO, "Using queue: %s, send status every %d seconds.",
         queue, status_interval);

  char outbox_dir[256];
  strcpy(outbox_dir, queue);
  strcat(outbox_dir, "/modem-outbox");
  MessageQueue outbox(outbox_dir);

  char inbox_dir[256];
  strcpy(inbox_dir, queue);
  strcat(inbox_dir, "/modem-inbox");
  MessageQueue inbox(inbox_dir);

  IMUProto imu_status = INIT_IMUPROTO;
  int imu_counter = 0;
  WindProto wind_status = INIT_WINDPROTO;
  int wind_counter = 0;
  ModemProto modem_status = INIT_MODEMPROTO;
  int modem_counter = 0;
  HelmsmanCtlProto helmsman_ctl = INIT_HELMSMANCTLPROTO;
  int helmsman_ctl_counter = 0;
  HelmsmanStatusProto helmsman_status = INIT_HELMSMAN_STATUSPROTO;
  int helmsman_status_counter = 0;
  FuelcellProto fuelcell_status = INIT_FUELCELLPROTO;
  int fuelcell_counter = 0;
  RemoteProto remote_status = INIT_REMOTEPROTO;
  remote_status.command = 1;
  int remote_counter = 0;

  time_t start_time = time(NULL);
  time_t last_remote_command = start_time;
  time_t status_time = start_time - status_interval + initial_timeout;
  time_t remote_cmd_time = start_time;
  int status_counter = 0;
  string status = default_status;

  bool shutdown_message_sent = false;
  while (!shutdown_message_sent) {
    // Wait 1s for something to read on stdin.
    fd_set read_fds, write_fds, except_fds;
    FD_ZERO(&read_fds);
    FD_SET(fileno(stdin), &read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    struct timeval timeout;
    timeout.tv_sec = remote_cmd_interval;
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
      {  // Check if this is a helmsman ctl.
        HelmsmanCtlProto status;
        unsigned int n = 0;
        if (sscanf(in, IFMT_HELMSMANCTLPROTO(&status, &n)) > 0 &&
            n == strlen(in)) {
          helmsman_ctl = status;
          helmsman_ctl_counter++;
        }
      }
      {  // Check if this is a helmsman status.
        HelmsmanStatusProto status;
        unsigned int n = 0;
        if (sscanf(in, IFMT_HELMSMAN_STATUSPROTO(&status, &n)) > 0 &&
            n == strlen(in)) {
          helmsman_status = status;
          helmsman_status_counter++;
        }
      }
      {  // Check if this is a fuelcell status.
        FuelcellProto status;
        unsigned int n = 0;
        if (sscanf(in, IFMT_FUELCELLPROTO(&status, &n)) > 0 &&
            n == strlen(in)) {
          fuelcell_status = status;
          fuelcell_counter++;
        }
      }
    }

    time_t now = time(NULL);
    while (inbox.NumMessages()) {
      string message;
      MessageQueue::MessageId id = inbox.GetMessage(0, &message);
      if (id != MessageQueue::kInvalidId) {
        inbox.DeleteMessage(id);
        if (message.size() > 0) {
          // Parse SMS message:
          // n = normal,      N = normal+ACK
          // d = docking,     D = docking+ACK
          // b = break,       B = break
          // o123 = override, O123 = override bearing 123 degrees + ACK
          // p = power off,   P = power off + ACK
          remote_status.timestamp_s = now;        
          remote_status.command = 0;
          if (tolower(message.at(0)) == 'n')  // Normal (autonomous)
            remote_status.command = kNormalControlMode;
          if (tolower(message.at(0)) == 'd')  // Docking
            remote_status.command = kDockingControlMode;
          if (tolower(message.at(0)) == 'b')  // Brake
            remote_status.command = kBrakeControlMode;
          if (tolower(message.at(0)) == 'o')  { // Override
            if (sscanf(message.substr(1).c_str(), "%lf",
                &remote_status.alpha_star_deg) == 1)
              remote_status.command = kOverrideSkipperMode;
          }
          if (tolower(message.at(0)) == 'p')  // Power cycle.
            remote_status.command = kPowerCycleMode;
          if (isupper(message.at(0))) {  // Status report / ack.
            char status_cstr[1024];
            sprintf(status_cstr, OFMT_REMOTEPROTO(remote_status));
            status = status_cstr;
            // Force sending a report in 5 seconds.
            status_time = now -status_interval + 5;
          }
          remote_counter++;
        }
      }
    }

    now = time(NULL);
    if (now < remote_cmd_time || now - remote_cmd_time >= remote_cmd_interval) {
      // Print the last received command.
      char out[1024];
      if (snprintf(out, sizeof(out), OFMT_REMOTEPROTO(remote_status)) <
          static_cast<int>(sizeof(out) - 1)) {
        puts(out);
      } else {
        syslog(LOG_ERR, "Remote status not sent because of short buffer");
      }
      remote_cmd_time = now;
    }

    if (quit || now < status_time || now - status_time >= status_interval) {
      if (quit) {
        status = "Shutting down!";
        shutdown_message_sent = true;
      }
      imu_counter = 0;
      wind_counter = 0;
      modem_counter = 0;
      helmsman_ctl_counter = 0;
      helmsman_status_counter = 0;
      fuelcell_counter = 0;
      remote_counter = 0;

      // Send status message.
      string status_message =
	BuildStatusMessage(now, imu_status, wind_status, modem_status,
                           helmsman_ctl, helmsman_status, fuelcell_status,
                           status);
      syslog(LOG_INFO, "Status message: \"%s\"", status_message.c_str());
      outbox.PushMessage(status_message);
      
      int retries = 10;
      while (outbox.NumMessages() > 0 && retries > 0) {
        // Wait for successfully sending the SMS status message.
        syslog(LOG_INFO, "Waiting for status message to be sent.");
        sleep(1);
        retries--;
      }
      if (retries > 0) {
        syslog(LOG_INFO, "Status message sent.");
      } else {
        syslog(LOG_ERR, "Status message still pending! "
                        "Is modem daemon running?");
      }
      
      status_time = now;
      status_counter++;
      status = default_status;
    }
  }
  syslog(LOG_INFO, "Status daemon shutdown.");
}
