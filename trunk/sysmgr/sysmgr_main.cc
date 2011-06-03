// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "lib/fm/fm.h"
#include "lib/fm/log.h"
#include "lib/util/stopwatch.h"
#include "sysmgr/adam.h"
#include "sysmgr/process_table.h"
#include "sysmgr/sysmon.h"
#include "sysmgr/watchdog.h"

// Hard-coded system configuration
#define ADAM_POWER_BUS_CPU 0
#define ADAM_POWER_BUS_MAIN 1

static const char *const short_options = "a:f:hm:nst:";
static const struct option long_options[] = {
  {"adam-device", 1, NULL, 'a'},
  {"cfg-file", 1, NULL, 'f'},
  {"help", 0, NULL, 'h'},
  {"sysmon-cfg", 0, NULL, 'm'},
  {"test-mode", 0, NULL, 'n'},
  {"stdio", 0, NULL, 's'},
  {"timeout", 1, NULL, 't'},
  {NULL, 0, NULL, 0},
};

static void PrintUsage(const char *name) __attribute__((noreturn));
static void PrintUsage(const char *name) {
  fprintf(stderr, "Usage: %s [options]\n", name);
  fprintf(stderr,
          "-a --adam-device  serial port for ADAM-4068 (default: none)\n"
          "-f --cfg-file     config file (default: /etc/sysmgr.conf)\n"
          "-h --help         print this message\n"
          "-n --test-mode    disable HW access for testing (default: enabled)\n"
          "-s --stdio        disable sysmon and take commands from stdio\n"
          "-t --timeout      system watchdog timeout (> 10s, default: 60s)\n"
          );
  exit(1);
}

// Allow for graceful shutdown on Ctr-C, etc.
static bool should_exit = false;
static void ExitHandler(int sig) {
    should_exit = true;
}

void InitPowerMgr(Adam &relay_controller, const char *adam_dev) {
   if (strcmp(adam_dev, "") == 0) {
     return;
  }

  if (!relay_controller.Init(adam_dev)) {
    FM_LOG_PERROR("opening ADAM serial port");
    FM_LOG_ERROR("could not initialize ADAM controller on port %s\n",
                 adam_dev);
    // Keep going anyway - but we won't be able to power-cycle
  } else {
    bool relay_state;
    if (relay_controller.GetRelayState(ADAM_POWER_BUS_CPU, &relay_state)) {
      if (relay_state) {
        FM_LOG_INFO("CPU bus relay is triggered - "
                    "probably recovering from power-cycling\n");
      }
    }
    // Force relays into know/desired state (no-op if they already are)
    relay_controller.SetRelayState(ADAM_POWER_BUS_CPU, false);
    relay_controller.SetRelayState(ADAM_POWER_BUS_MAIN, false);
  }
}

int main(int argc, char **argv) {
  // Defaults settings correspond to production environment
  bool test_mode = false; // use real hw access
  const char *adam_dev = "";
  const char *config_file = "/etc/sysmgr.conf";
  const char *sysmon_conf = "/etc/sysmon.conf";
  int timeout = 60; // 60 seconds
  bool use_sysmon = true;

  char *endptr=NULL;
  int next_opt;
  opterr = 0;
  do {
    next_opt = getopt_long(argc, argv, short_options,
                           long_options, NULL);
    switch (next_opt) {
      case 'n':
        test_mode = true;
        break;
      case 'a':
        adam_dev = optarg;
        break;
      case 'f':
        config_file = optarg;
        break;
      case 'm':
        sysmon_conf = optarg;
        break;
      case 's':
        use_sysmon = false;
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
        PrintUsage(argv[0]);
      default:   /* ignore invalid options */
        break;
    }
  } while (next_opt != -1);

  Watchdog watchdog(timeout, test_mode);

  // Re-scan argv/argc for logging system defaults
  FM::Init(argc, argv);

  // Handle graceful shutdown - mostly for testing
  // as in production, this should never happen...
  struct sigaction action;
  action.sa_handler = ExitHandler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGHUP, &action, NULL);

  // Load in the system process manifest from config file
  ProcessTable proc_table;
  if (!proc_table.Load(config_file)) {
    return 1; // In production, this would mean a reboot-loop!
  }

  Adam relay_controller;
  InitPowerMgr(relay_controller, adam_dev);

  TokenBuffer sysmon_cmd;
  SysMonClient sysmon(use_sysmon, timeout, sysmon_conf, proc_table);

  StopWatch timer;
  while (!should_exit) {
    timer.Set();

    // Care and feeding of all the tasks which we are managing
    proc_table.LoopOnce();

    // Handle commands from sysmon command channel
    // Wait at most 3s here
    // TODO(bsuter): try to use SIGCHLD to allow for longer wait
    // without loss of responsiveness
    if (sysmon.GetCommand(&sysmon_cmd, 3)) {
      FM_LOG_INFO("received '%s' from sysmon\n", sysmon_cmd.buffer);
      sysmon_cmd.Tokenize();
      if (sysmon_cmd.argc == 2 &&
          strcmp(sysmon_cmd.argv[0], "task-restart") == 0) {
          proc_table.KillTask(sysmon_cmd.argv[1]);
      } else if (sysmon_cmd.argc == 2 &&
                 strcmp(sysmon_cmd.argv[0], "power-cycle") ==0) {
        if (strcmp(sysmon_cmd.argv[1], "power-bus-cpu") == 0) {
          if (!relay_controller.Pulse(ADAM_POWER_BUS_CPU, 5)) {
            FM_LOG_ERROR("could not power cycle CPU power bus\n");
          }
          // Poof... we should be gone now
        } else if (strcmp(sysmon_cmd.argv[1], "power-bus-main") == 0) {
          if (!relay_controller.Pulse(ADAM_POWER_BUS_MAIN, 5)) {
            FM_LOG_ERROR("could not power cycle main power bus\n");
          }
        } else {
          FM_LOG_ERROR("Invalid bus ID for power-cycle: %s\n",
                  sysmon_cmd.argv[1]);
        }
      }
    }
    // Make sure we don't loop too fast and sleep at least 3s per iteration
    // - either here if not during read from cmd channel above.
    if (timer.Elapsed() < 2000) {
      sleep(3);
    }

    // Feed watchdog to prevent system from rebooting
    watchdog.Keepalive();
  }

  // Code below is for testing only - we should never
  // get here in production
  FM_LOG_INFO("shutting down...\n");
  proc_table.Shutdown();
  sysmon.Shutdown();
  // TODO(bsuter): remove this once we are done with kernel customizations
  // i.e. watchdog is compiled with NOWAYOUT.
  if (!test_mode) {
    sleep(2*watchdog.GetTimeout());
    FM_LOG_ERROR("hmm, we should have rebooted by now...\n");
  }
  return 0;
}
