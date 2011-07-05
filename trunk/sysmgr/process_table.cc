// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "sysmgr/process_table.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "lib/fm/fm.h"
#include "lib/fm/fm_messages.h"
#include "lib/fm/log.h"


ProcessTable::ProcessTable(): task_count_(0) { };

bool ProcessTable::Load(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    FM_LOG_PERROR("could not open process table file");
    return false;
  }

  while (true) {
    if (fgets(tasks_[task_count_].tokens.buffer,
              TB_MAX_BUFFER_SIZE, fp) == NULL) {
      break;
    }
    if(tasks_[task_count_].Init()) {
      task_count_++;
    }
  }
  fclose(fp);
  FM_LOG_INFO("loaded %d tasks from '%s'\n", task_count_, filename);
  return true;
}

void ProcessTable::KillTask(const char *taskname) {
  for (int i = 0; i < task_count_; i++) {
    if (strcmp(tasks_[i].name, taskname) != 0) {
      continue;
    }
    if (tasks_[i].pid != -1) {
      kill(tasks_[i].pid, SIGTERM);
      sleep(3);
      kill(tasks_[i].pid, SIGKILL);
      // We assume that Reap();Up(); is called in a continuous loop,
      // so all we need to do is kill in order to restart.
      // No matter how long it takes for the task to actuall go down.
    }
    break;
  }
}

void ProcessTable::Shutdown() {
  // Ask nicely at first
  for (int i = 0; i < task_count_; i++) {
    if (tasks_[i].pid != -1) {
      kill(tasks_[i].pid, SIGTERM);
    }
  }
  sleep(3);
  Reap();
  // Be more firm with task that have not gotten the hint yet.
  for (int i = 0; i < task_count_; i++) {
    if (tasks_[i].pid != -1) {
      kill(tasks_[i].pid, SIGKILL);
    }
  }
}

void ProcessTable::LoopOnce() {
  Reap();
  Up();
}

void ProcessTable::Up() {
  // Bring up any tasks which are currently down
  for (int i = 0; i < task_count_; i++) {
    if (tasks_[i].pid == -1) {
      tasks_[i].pid = LaunchTask(tasks_[i]);
    }
  }
}

void ProcessTable::Reap() {
  for (int i = 0; i < task_count_; i++) {
    if (tasks_[i].pid != -1) {
      // Check if this task has gone down
      pid_t pid = waitpid(tasks_[i].pid, NULL, WNOHANG);
      if (pid == tasks_[i].pid) {
        tasks_[i].pid = -1;
      }
    }
  }
}


pid_t ProcessTable::LaunchTask(const ProcessInfo &task_info) {
  pid_t pid = fork();

  if (pid != 0) { // we are the parent
    if (pid == -1) {
      FM_LOG_PERROR("fork failed");
    }
    // Return -1, will retry to launch again in next iteration.
    return pid;
  }

  // We are the new child process here

  // Since we may be setting signals in the parent (typically sysmgr),
  // clear them again back to defaults before starting
  // the new task.
  sigset_t empty;
  sigemptyset(&empty);
  sigprocmask(SIG_SETMASK, &empty, NULL);

  execv(task_info.tokens.argv[0], task_info.tokens.argv);
  FM_LOG_PERROR("exec failed");
  FM_LOG_ERROR("could not start %s\n", task_info.name);
  exit(1);
}

bool ProcessTable::ProcessInfo::Init() {
  pid = -1; // Process is starting out as down

  // Needs at least 3 arguments: task name, timeout and executable path
  if (!tokens.Tokenize() || tokens.argc < 3) {
    return false;
  }
  name = tokens.argv[0];
  char *endptr;
  timeout = strtol(tokens.argv[1], &endptr, 0);
  if (endptr[0] != '\0') {
    return false;
  }
  // Move remaining arguments (plus NULL) down to argv[0] for execv
  memmove(tokens.argv, tokens.argv + 2, (tokens.argc + 1) * sizeof(char *));
  tokens.argc -= 2;
  return true;
}
