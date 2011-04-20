// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LOG_FM_FM_H__
#define LOG_FM_FM_H__

#include "lib/fm/constants.h"
#include "lib/fm/mon_client.h"
#include "lib/util/stopwatch.h"

// Task level fault-management interface
class FM {
 public:

  // Configure FM module by extracting some long options from cmdline
  static bool Init(int argc, char **argv);

  // Trigger SW watchdog keepalive message to system monitor
  static void Keepalive();

  // Explicitly report task status to system monitor (optional)
  static void SetStatus(FM_STATUS status, const char *msg);
  // Increment task iteration counter (optional)
  static void Iteration();

  // Generate a system log message
  static void Log(FM_LOG_LEVELS level, const char *msg);

  static const MonClient &Monitor() { return mon_client_; }

 private:
  static MonClient mon_client_;

  static StopWatch interval_;
  static long iterations_;
  static long log_counts_[FM_LEVEL_MAX];

  // Settings
  static long timeout_sec_;
  static bool debug_;
  static bool logtostderr_;
  static bool use_syslog_;
  static char task_name_[30];
  static char mon_socket_name_[256];
};

#endif // LOG_FM_FM_H__
