// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SYSMGR_PROCESS_TABLE_H__
#define SYSMGR_PROCESS_TABLE_H__

#include <unistd.h>

#include "lib/util/token_buffer.h"

#define PT_MAX_TASKS 30

// Maintain manifest of tasks to run as unix processes.
class ProcessTable {
 public:

  ProcessTable();

  // Load process table config from file.
  bool Load(const char *filename);

  // Bring down a particular, named task for restart.
  void KillTask(const char *taskname);

  // Bring down all taks (test use only).
  void Shutdown();

  // Must be called from the application main loop during each iteration.
  void LoopOnce();

 private:
  // Represents a single task record in the table.
  struct ProcessInfo {
    const char *name;
    int timeout;
    pid_t pid;
    TokenBuffer tokens;

    bool Init();
  };

  // Scan process table for tasks which need to be brought up.
  void Up();
  // Scan process table for tasks which are no longer running.
  void Reap();

  // Helper function to launch one task.
  pid_t LaunchTask(const ProcessInfo &task_info);

  ProcessInfo tasks_[PT_MAX_TASKS];
  int task_count_;
};


#endif // SYSMGR_PROCESS_TABLE_H__
