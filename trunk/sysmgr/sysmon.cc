// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/sysmon.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

SysMon::SysMon(int timeout_s, int sysmgr_pipe) : timeout_s_(timeout_s) {
  // TODO(bsuter): use assert FM_ASSERT after integration with log.h
  pipe_ = fdopen(sysmgr_pipe, "w");
  setvbuf(pipe_, NULL, _IONBF, 0);
  fprintf(stderr, "starting sysmon\n");
}

int SysMon::Run() {
  while(1) {
    fprintf(pipe_, "keepalive\n");
    // TODO(bsuter): do the real monitoring work...
    sleep(timeout_s_ / 3);
  }
  // should never get here...
  return -1;
}

SysMonClient::SysMonClient(bool use_sysmon, int timeout_s) :
    use_sysmon_(use_sysmon),
    timeout_s_(timeout_s),
    sysmon_pid_(-1),
    sysmon_pipefd_(-1) {
  // If sysmon is disabled, read commands from stdin instead
  if (!use_sysmon) {
    sysmon_reader_.Init(fileno(stdin), false);
  }
};

bool SysMonClient::GetCommand(TokenBuffer *sysmon_cmd, int timeout_s) {
  ManageProc(); // Make sure the SysMon process is healty and running

  Reader::ReadState status = sysmon_reader_.ReadLine(
      sysmon_cmd->buffer,
      sizeof(sysmon_cmd->buffer),
      timeout_s * 1000);

  // Do nothing in error cases - if it persists for long
  // the sysmon task will eventually time out and be
  // killed/restarted.
  switch (status) {
    case Reader::READ_TIMEOUT:
      break; // this is fairly expected
    case Reader::READ_EOF:
      fprintf(stderr, "EOF from sysmon channel!\n");
      break;
    case Reader::READ_OVERFLOW:
      fprintf(stderr, "Line buffer overflow on sysmon channel!\n");
      break;
    case Reader::READ_ERROR:
      perror("Error on read from sysmon channel");
      break;
    case Reader::READ_OK:
      sysmon_timer_.Set(); // Reset sysmon keepalive timer
      return true;
  }
  return false;
}

void SysMonClient::Shutdown() {
  if (use_sysmon_ && sysmon_pid_ != -1) {
    kill(sysmon_pid_, SIGTERM);
    kill(sysmon_pid_, SIGKILL);
  }
}

void  SysMonClient::ManageProc() {
  if (!use_sysmon_) {
    return;
  }
  if (sysmon_pid_ != -1) {
    if (sysmon_timer_.Elapsed() > timeout_s_ * 1000) {
      // sysmon must be hanging - restart
      kill(sysmon_pid_, SIGTERM);
      sleep(3);
      kill(sysmon_pid_, SIGKILL);
      fprintf(stderr, "sysmon timeout - killing...\n");
    }
    // Check if sysmon has exited (killed or crashed)
    if (waitpid(sysmon_pid_, NULL, WNOHANG) == sysmon_pid_) {
      sysmon_pid_ = -1;
      close(sysmon_pipefd_);
      fprintf(stderr, "sysmon exit detected\n");
    }
  }
  // Restart sysmon, if down
  if (sysmon_pid_ == -1) {
    if (LaunchSysmon()) {
      sysmon_reader_.Init(sysmon_pipefd_, true);
      sysmon_timer_.Set();
      fprintf(stderr, "sysmon started\n");
    }
  }
}

bool  SysMonClient::LaunchSysmon() {
  int pipes[2];

  if (pipe(pipes) == -1) {
    return false;
  }

  sysmon_pid_ = fork();
  if (sysmon_pid_ == -1) {
    return false;
  }
  // Give reading end back to sysmgr
  sysmon_pipefd_ = pipes[0];

  if (sysmon_pid_ == 0) {
    // We are the new sysmon task

    // Reset signal handlers which have been set by parent
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGHUP, SIG_DFL);

    SysMon sysmon(timeout_s_, pipes[1]);
    exit(sysmon.Run());
  } else {
    // We are still the old sysmgr task
    return true;
  }
}
