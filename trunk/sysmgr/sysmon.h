// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SYSMGR_SYSMON_H__
#define SYSMGR_SYSMON_H__

#include <stdio.h>
#include <unistd.h>

#include "lib/util/stopwatch.h"
#include "lib/util/reader.h"
#include "lib/util/token_buffer.h"

// System Monitor.
//
// Runs as a coprocessor to sysmgr and implements the system
// fault management policies (monitoring for timeouts, event
// threshold crossing alarms and corrective actions: restarts,
// power-cycling).
class SysMon {
 public:
  SysMon(int timeout_s, int sysmgr_pipe);

  int Run();

 private:
  int timeout_s_;
  FILE *pipe_;
};

// Encapsulates management of SysMon coprocessor interface in SysMgr task.
//
// If necessary forks off new SysMon process, reads commands from its
// IPC pipe and restarts process if not activity is found.
class SysMonClient {
 public:
  SysMonClient(bool use_sysmon, int timeout_s);

  // Do one iteration of the client loop and wait for a command
  // from SysMon. Return true, if there was a command.
  bool GetCommand(TokenBuffer *sysmon_cmd, int timeout_s);

  // For testing purposes: clean shutdown of SysMon process.
  void Shutdown();

 private:
  // Make sure that the SysMon co-processor task is running and healty
  // and if necessary (re)start it.
  void ManageProc();

  // Run a SysMon instance as its own process
  bool LaunchSysmon();

  bool use_sysmon_;
  int timeout_s_;
  pid_t sysmon_pid_;
  int sysmon_pipefd_;
  Reader sysmon_reader_;
  StopWatch sysmon_timer_;
};

#endif // SYSMGR_SYSMON_H__
