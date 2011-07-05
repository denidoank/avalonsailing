// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "lib/fm/device_monitor.h"
#include "lib/fm/fm.h"
#include "lib/fm/log.h"
#include "lib/util/token_buffer.h"

// Dummy client for testing sysmon message processing
// and fault detection logic.
// Cmdline args are a list of devices to instantiate.
// Reads a sequence of commands from stdin.

int main(int argc, char **argv) {
  int optind = FM::Init(argc, argv);
  argc -= optind;
  argv += optind;

  std::map<std::string, DeviceMonitor *> mon_map;

  for (int i = 0; i < argc; i++) {
    mon_map[argv[i]] = new DeviceMonitor(argv[i], 1);
  }

  TokenBuffer cmd;

  while (fgets(cmd.buffer, sizeof(cmd.buffer), stdin) != NULL) {
    cmd.Tokenize();
    if (cmd.argc < 2) {
      FM_LOG_ERROR("insufficient arguments for operation");
      continue;
    }
    int delay_s = atoi(cmd.argv[0]);
    std::string op(cmd.argv[1]);

    if (op == "keepalive") {
      sleep(delay_s);
      FM::Keepalive();
    } else if (mon_map.find(op) != mon_map.end()) {
      if (cmd.argc < 5) {
        FM_LOG_ERROR("insufficient argument for device operation");
        continue;
      }
      DeviceMonitor *dev = mon_map[op];
      for (int i = 0; i < atoi(cmd.argv[2]) - 1; i++) {
        dev->Ok();
      }
      for (int i = 0; i < atoi(cmd.argv[3]); i++) {
        dev->CommError();
      }
      for (int i = 0; i < atoi(cmd.argv[4]); i++) {
        dev->DevError();
      }
      sleep(delay_s);
      dev->Ok();
    }
  }
  FM_LOG_FATAL("crash...");
}
