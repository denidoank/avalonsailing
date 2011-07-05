// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/sysmon.h"

#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "lib/fm/fm_messages.h"
#include "lib/fm/log.h"
#include "lib/util/strings.h"
#include "sysmgr/active_entity.h"
#include "sysmgr/passive_entity.h"
#include "sysmgr/process_table.h"

static const char *defaults[][2] = {{"socket", "/tmp/sysmon"},
                                    {"alarm_log", "/tmp/alarms.log"},
                                    {"alarm_log_timeout_s", "600"},
                                    {NULL, NULL}};

SysMon::SysMon(int timeout_s, int sysmgr_pipe, const char *config_file,
               const ProcessTable &proc_table)
  : timeout_s_(timeout_s) {
  // LoadProperties(defaults, config_file, &properties_);

  std::string socket;
  FM_ASSERT(properties_.GetKeyVals().Get("socket", &socket));
  OpenSocket(socket.c_str());

  CreatePassiveEntities();
  CreateTaskEntities(proc_table);
  CreateDeviceEntities();
  LoadAlarmLog();

  // Create FILE from pipe fd
  FM_ASSERT_PERROR((pipe_ = fdopen(sysmgr_pipe, "w")) != NULL);
  FM_ASSERT_PERROR(setvbuf(pipe_, NULL, _IONBF, 0) == 0);


  FM_LOG_INFO("starting sysmon\n");
}

SysMon::~SysMon() {
  for (EntityMap::iterator i = entities_.begin(); i != entities_.end(); ++i) {
    delete i->second;
  }
}

int SysMon::Run() {
  char buffer[FM_MSG_SIZE];
  char name[FM_NAME_SIZE];

  Schedule(timeout_s_ / 3);
  while(1) {
    if (GetMessage(buffer, sizeof(buffer),
                   DelayedEvent::GetWaitTimeS(3) * 1000)) {
        std::string msg_string = buffer;
        KeyValuePair msg(msg_string);
        std::string name;
        FM_LOG_DEBUG("%s", buffer);
        if (msg.Get("name", &name)) {
          EntityMap::iterator it = entities_.find(name);
          if ( it != entities_.end()) {
            it->second->ProcessMessage(msg);
          } else {
            FM_LOG_ERROR("Unknown entity '%s'", name.c_str());
          }
        }
      } else {
        DelayedEvent::Dispatch(); // do all the scheduled events here
      }
  }
  // should never get here...
  return -1;
}

void SysMon::Handle() {
  ExecuteAction("keepalive");
}

void SysMon::SendSMS(const std::string &message) {
  FM_LOG_INFO("sending SMS: '%s'", message.c_str());
  StoreAlarmLog();
}

void SysMon::ExecuteAction(const char *cmd) {
  fprintf(pipe_, "%s\n", cmd);
  Schedule(timeout_s_ / 3);
}

bool SysMon::LoadAlarmLog() {
  std::string filename;
  if (!properties_.GetKeyVals().Get("alarm_log", &filename)) {
    return false;
  }
  FILE *fp = fopen(filename.c_str(), "r");
  if (fp == NULL) {
    FM_LOG_PERROR("could not open alarm log");
    return false;
  }
  struct stat file_stats;
  if (fstat(fileno(fp), &file_stats) != -1) {
    time_t now = time(NULL);
    long log_timeout = properties_.Get("alarm_log_timeout_s", 60L * 10);
    if (now - file_stats.st_mtime > log_timeout) {
      FM_LOG_INFO("alarm log file is older than %lds - ignoring content",
                  log_timeout);
      return false;
    }
  } else {
    FM_LOG_PERROR("could not stat alarm log");
    return false;
  }
  char buffer[256];
  while (true) {
    if(fgets(buffer, sizeof(buffer), fp) == NULL) {
      break;
    }
     int len = strlen(buffer);
    // Strip newlines.
    if (buffer[len - 1] == '\n') {
      buffer[len - 1] = '\0';
    }
    KeyValuePair entity_log(buffer);
    std::string name;
    if (entity_log.Get("entity", &name)) {
      EntityMap::iterator it = entities_.find(name);
      if ( it != entities_.end()) {
        it->second->LoadState(entity_log);
      } else {
        FM_LOG_ERROR("No entity '%s'", name.c_str());
      }
    }  else {
      FM_LOG_ERROR("no entity name found in record '%s'", buffer);
    }
  }
  fclose(fp);
  return true;
}

bool SysMon::StoreAlarmLog() {
  std::string filename;
  if (!properties_.GetKeyVals().Get("alarm_log", &filename)) {
    return false;
  }
  FILE *fp = fopen(filename.c_str(), "w");
  if (fp == NULL) {
    FM_LOG_PERROR("could not open alarm log");
    return false;
  }
  for (EntityMap::iterator it = entities_.begin();
       it != entities_.end(); ++it) {
    KeyValuePair entity_data;
    it->second->StoreState(&entity_data);
    fputs(entity_data.ToString(false).c_str(), fp);
    fputc('\n', fp);
  }
  fsync(fileno(fp));
  fclose(fp);
}

void SysMon::OpenSocket(const char *socket_name) {
  FM_ASSERT_PERROR((msg_socket_ = socket(AF_UNIX, SOCK_DGRAM, 0))
                   != -1);

  struct sockaddr_un address;
  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy(address.sun_path, socket_name);
  unlink(socket_name);
  FM_ASSERT_PERROR(bind(msg_socket_, (const struct sockaddr *) &address,
                        sizeof(address)) != -1);
  FM_LOG_INFO("Open socket %s", socket_name);
}

void SysMon::CreatePassiveEntities() {
  {
  std::string name = "controller-power";
  RecoveryPolicy policy(properties_.Get(name + ".repair_time", 600L), // 10min
                        properties_.Get(name + ".retry_time", 86400L)); // 24h
  SystemEntity *system;
  FM_ASSERT(system = new SystemEntity(name.c_str(),
                                      "power-cycle power-bus-cpu",
                                      policy, this));
  entities_[name] = system;
  }
 {
  std::string name = "motors-power";
  RecoveryPolicy policy(properties_.Get(name + ".repair_time", 600L), // 10min
                        properties_.Get(name + ".retry_time", 43200L)); // 12h
  PowerBusEntity *motors;
  FM_ASSERT(motors = new PowerBusEntity(name.c_str(),
                                        "power-cycle power-bus-main",
                                        policy, this));
  entities_[name] = motors;
  }
 {
  std::string name = "sensors-power";
  RecoveryPolicy policy(properties_.Get(name + ".repair_time", 300L), // 5min
                        properties_.Get(name + ".retry_time", 21600L)); // 6h
  PowerBusEntity *sensors;
  FM_ASSERT(sensors = new PowerBusEntity(name.c_str(),
                                         "power-cycle power-bus-aux",
                                         policy, this));
  entities_[name] = sensors;
  }
}

void SysMon::CreateTaskEntities(const ProcessTable &procTable) {
  EntityList recovery_order;
  recovery_order.push_back(entities_["controller-power"]);
  for (int i = 0; i < procTable.task_count_; i++) {
    std::string name = procTable.tasks_[i].name;
    long timeout = procTable.tasks_[i].timeout;

    TaskEntity *task;
    RecoveryPolicy policy(properties_.Get(name + ".repair_time", 10L),
                          properties_.Get(name + ".retry_time", 60L));
    FM_ASSERT(task = new TaskEntity(name.c_str(), timeout,
                                    properties_.Get(name + ".alarm_hold_time",
                                                    2 * timeout),
                                    policy, recovery_order, this, properties_));
    entities_[name] = task;
  }
  FM_LOG_INFO("created %d monitoring task entities", procTable.task_count_);
}

void SysMon::CreateDeviceEntities() {
  std::string value;
  if (!properties_.GetKeyVals().Get("devices", &value)) {
      FM_LOG_ERROR("no devices registered");
      return;
    }
  std::vector<std::string> dev_list;
  if (!SplitString(value, ',', &dev_list)) {
    FM_LOG_ERROR("devices is not a proper ',' separated list: '%s'",
                 value.c_str());
    return;
  }

  for (std::vector<std::string>::iterator it = dev_list.begin();
       it != dev_list.end(); ++it) {
    std::string &name = *it;

    if (!properties_.GetKeyVals().Get(name + ".recovery_order", &value)) {
      FM_LOG_ERROR("no recovery_order for %s", name.c_str());
      continue;
    }
    std::vector<std::string> recovery_list;
    if (!SplitString(value, ',', &recovery_list)) {
      FM_LOG_ERROR("recovery_order is not a proper ',' separated list: '%s'",
                   value.c_str());
      return;
    }

    EntityList recovery_order;
    for (std::vector<std::string>::iterator it = recovery_list.begin();
         it != recovery_list.end(); ++it) {
      EntityMap::iterator mit = entities_.find(*it);
      if (mit == entities_.end()) {
        FM_LOG_ERROR("Could not find entity '%s'", it->c_str());
      }
      recovery_order.push_back(mit->second);
    }
    RecoveryPolicy policy(properties_.Get(name + ".repair_time", 10L),
                          properties_.Get(name + ".retry_time", 60L));
    DeviceEntity *device;
    FM_ASSERT(device = new DeviceEntity(name.c_str(),
                                        properties_.Get(name + ".timeout", 10L),
                                        properties_.Get(
                                            name + ".alarm_hold_time", 30L),
                                        policy, recovery_order,
                                        this, properties_));
    entities_[name] = device;
  }
}

bool SysMon::GetMessage(char *buffer, int size, int timeout_ms) {
  pollfd poll_request;

  poll_request.fd = msg_socket_;
  poll_request.events = POLLIN;
  int rc;
  FM_ASSERT_PERROR((rc = poll(&poll_request, 1, timeout_ms)) != -1);
  if (rc == 0) { // timeout
    return false;
  }
  FM_ASSERT_PERROR(recv(msg_socket_, buffer, size, 0) != -1);
  return true;
}

SysMonClient::SysMonClient(bool use_sysmon, int timeout_s, const char *config_file,
                           const ProcessTable &proc_table) :
    use_sysmon_(use_sysmon),
    timeout_s_(timeout_s),
    config_file_(config_file),
    proc_table_(proc_table),
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
      FM_LOG_ERROR("EOF from sysmon channel!\n");
      break;
    case Reader::READ_OVERFLOW:
      FM_LOG_ERROR("Line buffer overflow on sysmon channel!\n");
      break;
    case Reader::READ_ERROR:
      FM_LOG_PERROR("Error on read from sysmon channel");
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

    SysMon sysmon(timeout_s_, pipes[1], config_file_, proc_table_);
    exit(sysmon.Run());
  } else {
    // We are still the old sysmgr task
    return true;
  }
}
