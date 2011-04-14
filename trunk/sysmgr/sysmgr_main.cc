// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sysmgr/watchdog.h>

static const char *const short_options = "f:hnt:";
static const struct option long_options[] = {
  {"cfg-file", 1, NULL, 'f'},
  {"help", 0, NULL, 'h'},
  {"test-mode", 0, NULL, 'n'},
  {"timeout", 1, NULL, 't'},
  {NULL, 0, NULL, 0},
};

static void PrintUsage(const char *name) __attribute__((noreturn));
static void PrintUsage(const char *name) {
  fprintf(stderr, "Usage: %s [options]\n", name);
  fprintf(stderr,
          "-f --cfg-file     config file (default: /etc/sysmgr.conf\n"
          "-h --help         print this message\n"
          "-n --test-mode    disable HW access for testing (default: enabled)\n"
          "-t --timeout      system watchdog timeout (> 10s, default: 60s)\n"
          );
  exit(1);
}

// Allow for graceful shutdown on Ctr-C, etc.
static bool should_exit = false;
static void ExitHandler(int sig) {
    should_exit = true;
}

int LaunchSysmon() {
  pid_t pid;

  pid = fork();
  if (pid == -1) {
    return -1;
  }

  if (pid == 0) {
    // we are the new sysmon task
    fprintf(stderr, "this is sysmon...\n");
    // TODO(bsuter): do real stuff...
    sleep(15);
    fprintf(stderr, "sysmon exiting...\n");
    exit(0);
  } else {
    // we are still the old sysmgr task
    return 0;
  }
}

int main(int argc, char **argv) {

  // Defaults settings correspond to production environment
  bool test_mode = false; // use real hw access
  const char *config_file = "/etc/sysmgr.conf";
  int timeout = 60; // 60 seconds

  char *endptr=NULL;
  int next_opt;
  do {
    next_opt = getopt_long(argc, argv, short_options,
                           long_options, NULL);
    switch (next_opt) {
      case 'n':
        test_mode = true;
      case 'f':
        config_file = optarg;
        break;
      case 't':
        timeout = strtol(optarg, &endptr, 0);
        if (timeout < 10 ||  endptr[0] != '\0') {
          fprintf(stderr,
                  "invalid timeout (value must be larger than 10s)\n");
          PrintUsage(argv[0]);
        }
        break;
      case -1:   /* Done with options */
        break;
      case 'h':
      default:   /* invalid options */
          PrintUsage(argv[0]);
    }
  } while (next_opt != -1);

  Watchdog watchdog(timeout, test_mode);

  // Handle graceful shutdown - mostly for testing
  // as in production, this should never happen...
  struct sigaction action;
  action.sa_handler = ExitHandler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGHUP, &action, NULL);

  LaunchSysmon();
  while (!should_exit) {
    // TODO(bsuter): do some real work here...
    sleep(watchdog.GetTimeout()/3);
    watchdog.Keepalive();
  }
  fprintf(stderr, "shutting down...\n");
  if (!test_mode) {
    sleep(2*watchdog.GetTimeout());
    fprintf(stderr, "hmm, we should have rebooted by now...\n");
  }
}
