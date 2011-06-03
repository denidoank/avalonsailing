// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SYSMGR_SYSMON_H__
#define SYSMGR_SYSMON_H__

#include <map>
#include <string>

#include <stdio.h>
#include <unistd.h>

#include "io/ipc/key_value_pairs.h"
#include "lib/util/delayed_event.h"
#include "lib/util/reader.h"
#include "lib/util/stopwatch.h"
#include "lib/util/token_buffer.h"

class ProcessTable;
class Entity;

typedef std::map<std::string, Entity *> EntityMap;

// System Monitor.
//
// Runs as a coprocessor to sysmgr and implements the system
// fault management policies (monitoring for timeouts, event
// threshold crossing alarms and corrective actions: restarts,
// power-cycling).
class SysMon : public DelayedEvent {
 public:
  SysMon(int timeout_s, int sysmgr_pipe, const char *config_file,
         const ProcessTable &proc_table);
  ~SysMon();

  int Run();

  void Handle();
  void SendSMS(const std::string &message);
  void ExecuteAction(const char *cmd);

  // Load alarm & recovery state from a presistent log file at startup
  bool LoadAlarmLog();
  // Save alarm & recovery state in a file
  bool StoreAlarmLog();

 private:
  void OpenSocket(const char *socket_name);
  void CreateTaskEntities(const ProcessTable &procTable);

  bool GetMessage(char *buffer, int size, int timeout_ms);

  EntityMap entities_;
  KeyValuePair properties_;

  int msg_socket_;
  FILE *pipe_;
  const long timeout_s_;
};

// Encapsulates management of SysMon coprocessor interface in SysMgr task.
//
// If necessary forks off new SysMon process, reads commands from its
// IPC pipe and restarts process if not activity is found.
class SysMonClient {
 public:
  SysMonClient(bool use_sysmon, int timeout_s, const char *config_file,
                           const ProcessTable &proc_table);

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

  const bool use_sysmon_;
  const int timeout_s_;
  const char *config_file_;
  const ProcessTable &proc_table_;

  pid_t sysmon_pid_;
  int sysmon_pipefd_;
  Reader sysmon_reader_;
  StopWatch sysmon_timer_;
};

#endif // SYSMGR_SYSMON_H__
