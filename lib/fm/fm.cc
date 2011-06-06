// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/fm/fm.h"

#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "lib/fm/fm_messages.h"
#include "lib/fm/log.h"

void fm_log(enum FM_LOG_LEVELS level, const char *file, int line,
            const char *fmt, ...)
{
  va_list arg_ptr;
  char buffer[256];
  const char *basename = file;

  for (int i = 0; file[i] != '\0'; i++) {
    if (file[i] == '/') {
      basename = file + i + 1;
    }
  }

  int prefix_size = snprintf(buffer, sizeof(buffer), "%s:%d ",
                             basename, line);
  prefix_size = prefix_size < sizeof(buffer) - 1 ?
      prefix_size : sizeof(buffer) - 1;

  va_start(arg_ptr, fmt);
  vsnprintf(buffer + prefix_size, sizeof(buffer) - prefix_size, fmt, arg_ptr);
  va_end(arg_ptr);

  FM::Log(level, buffer);
  if (level == FM_LEVEL_FATAL) {
    FM::SetStatus(FM_STATUS_FAULT, buffer);
	abort();
  }
}

// FM module static data
MonClient FM::mon_client_;
StopWatch FM::interval_;
long FM::iterations_ = 0;
long FM::log_counts_[FM_LEVEL_MAX];

// Settings & defaults
long FM::timeout_sec_ = 60;
bool FM::debug_ = false;
bool FM::logtostderr_ = false;
bool FM::use_syslog_ = true;
char FM::task_name_[30] = "";

int FM::Init(int argc, char **argv) {
  static const struct option long_options[] = {
    {"debug", 0, NULL, 0},
    {"logtostderr", 0, NULL, 0},
    {"no-syslog", 0, NULL, 0},
    {"sysmon-socket", 1, NULL, 0},
    {"task-name", 1, NULL, 0},
    {"timeout", 1, NULL, 0},
    {NULL, 0, NULL, 0},
  };
  int opt_index;
  char *endptr = NULL;

  strncpy(task_name_, argv[0], sizeof(task_name_));

  optind = 1; // reset in case somebody already did getopt
  opterr = 0; // turn off getopt error messages for options we don't care about
  while (getopt_long(argc, argv, "", long_options, &opt_index) != -1) {
    switch(opt_index) {
      case 0: // --debug
        debug_ = true;
        break;
      case 1: // --logtostderr
        logtostderr_ = true;
        break;
      case 2: // --no-syslog
        use_syslog_ = false;
        break;
     case 3: // --sysmon-socket
        mon_client_.SetDestination(optarg);
        break;
      case 4: // --task-name
        strncpy(task_name_, optarg, sizeof(task_name_));
        break;
      case 5: // -- timeout
        timeout_sec_ = strtol(optarg, &endptr, 0);
        if (timeout_sec_ < 10 ||  endptr[0] != '\0') {
          fprintf(stderr, "invalid timeout (value must be larger than 10s)\n");
          return -1;
        }
        break;
      default:
        break;
    }
  }
  int rc = optind;
  optind = 1; // reset argv iteration index

  if (use_syslog_) {
    openlog(task_name_, 0, LOG_USER);
  }

  FM_LOG_INFO("Fault management settings: task-name: %s, timeout %ld,"
              " debug: %c, logtostderr: %c, syslog: %c, sysmon socket: %s",
              task_name_,
              timeout_sec_,
              debug_ ? 'y' : 'n',
              logtostderr_ ? 'y' : 'n',
              use_syslog_ ? 'y' : 'n',
              mon_client_.GetDestination());

  return rc;
}

void FM::Keepalive(const CustomVar *variables) {
  rusage usage_data;
  if (getrusage(RUSAGE_SELF, &usage_data) == -1) {
    FM_LOG_PERROR("rusage failure");
    return;
  }
  long cpu_time = usage_data.ru_utime.tv_sec + usage_data.ru_stime.tv_sec;
  char buffer[256];
  if (variables != NULL) {
    for (int i = 0, pos = 0;
         variables[i].name != NULL && pos < sizeof(buffer);
         i++ ) {
      pos += snprintf(buffer + pos, sizeof(buffer) - pos,
                      " %s:%.3f", variables[i].name, variables[i].value);
    }
  } else {
    buffer[0] = '\0';
  }

  mon_client_.SendMonMsg(FM_MSG_TASK_KEEPALIVE,
                         task_name_,
                         interval_.Elapsed(),
                         iterations_,
                         log_counts_[FM_LEVEL_ERROR],
                         log_counts_[FM_LEVEL_WARNING],
                         log_counts_[FM_LEVEL_INFO],
                         cpu_time,
                         usage_data.ru_maxrss,
                         buffer);
  interval_.Set();
  memset(log_counts_, '\0', sizeof(log_counts_));
}

void FM::SetStatus(FM_STATUS status, const char *msg) {
  static const char *status_names[] = FM_STATUS_NAMES_;
  mon_client_.SendMonMsg(FM_MSG_STATUS,
                         task_name_,
                         status_names[status],
                         msg);
  Keepalive(); // flush out keepalive stats as well
}

void FM::Iteration() {
  iterations_++;
}

void FM::Log(FM_LOG_LEVELS level, const char *msg) {
  static int syslog_levels[] = FM_LEVEL_SYSLOG_MAP_;
  static char level_names[] = FM_LEVEL_NAMES_;
  if (level == FM_LEVEL_DEBUG && !debug_) {
    return;
  }
  if (use_syslog_) {
    syslog(syslog_levels[level], "%s", msg);
  }
  if (logtostderr_) {
    fprintf(stderr, "%c %s\n", level_names[level], msg);
  }
  log_counts_[level]++;
}
