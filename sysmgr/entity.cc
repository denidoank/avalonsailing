// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/entity.h"

#include <stdio.h>
#include <string.h>

#include "lib/fm/fm_messages.h"
#include "lib/fm/log.h"
#include "sysmgr/sysmon.h"

Entity::~Entity() {}

void Entity::Handle() {
  if (entity_alarm_.IsSet()) {
    this->Recover();
  } else {
    if (entity_alarm_.TrySet("timeout")) {
      sysmon_->SendSMS(entity_alarm_.ToString());
      this->Recover();
    }
  }
}

void Entity::ProcessMessage(const KeyValuePair &msg) {
  bool extracted = true;
  std::string  status, comment;
  extracted &= msg.Get("status", &status);
  extracted &= msg.Get("msg", &comment);

  if (extracted) {
  FM_LOG_INFO("received status %s", status.c_str());
    if (entity_alarm_.IsSet() == false) {
      if (status == "FAULT") {
        entity_alarm_.TrySet(comment.c_str());
        sysmon_->SendSMS(entity_alarm_.ToString());
        Cancel(); // cancel pending timeout
        this->Recover();
      }
    } else { // currently in FAULT state
      if (status == "OK"
          && CheckMonStatus()
          && entity_alarm_.TryClear()) {
        sysmon_->SendSMS(entity_alarm_.ToString()); // record clearing of alarm
      }
    }
    if (!entity_alarm_.IsSet()) {
      Schedule(timeout_s_);
    }
  }
}

bool Entity::CheckMonStatus() {
  bool status = true;
  for (MonVarList::iterator it = mon_var_list_.begin();
       it != mon_var_list_.end(); ++it) {
    status &= (*it)->CheckStatus();
  }
  return true;
}

void Entity::LoadState(const KeyValuePair &data) {
  entity_alarm_.LoadState(data);
  // TODO(bsuter): restore recovery state.
}

void Entity::StoreState(KeyValuePair *data) {
  data->Add("entity", name_);
  entity_alarm_.StoreState(data);
  // TODO(bsuter): store recovery state.
}

TaskEntity::TaskEntity(const char *name, int timeout_s,
                       SysMon *sysmon) :
    Entity(name, timeout_s, sysmon) {
  mon_var_list_.resize(VAR_MEM + 1);
  // TODO(bsuter): use properties to configure monitoring thresholds.
  mon_var_list_[VAR_ERRORS] = new MonVar(0.5, 10, true, 2 * timeout_s,
                                         "ERROR", "error rate > 0.5/s");
  mon_var_list_[VAR_LOG] = new MonVar(2, 10, true, 2 * timeout_s,
                                      "LOG", "log rate > 2/s");
  mon_var_list_[VAR_CPU] = new MonVar(0.8, 10, true, 2 * timeout_s,
                                      "CPU", "avg usage > 0.8");
  mon_var_list_[VAR_MEM] = new MonVar(1024 * 1024 * 10.0, 3, true,
                                      2 * timeout_s,
                                      "MEM", "usage > 10MB");
}

TaskEntity::~TaskEntity() {
  for (MonVarList::iterator it = mon_var_list_.begin(); it != mon_var_list_.end();
       ++it) {
    delete *it;
  }
}

void TaskEntity::ProcessMessage(const KeyValuePair &msg) {
  bool extracted = false;
  long time_ms, iterations, errors, warnings, info, cpu, memory;

  if (extracted) {
    // TODO(bsuter): implement monitor for rate type variables
    if (time_ms > 0) {
      mon_var_list_[VAR_ERRORS]->TestValue(1000 * errors / time_ms,
                                           sysmon_);
      mon_var_list_[VAR_LOG]->TestValue(1000 * (errors + warnings + info) /
                                        time_ms, sysmon_);
      mon_var_list_[VAR_CPU]->TestValue(1000 * cpu / time_ms, sysmon_);
      mon_var_list_[VAR_MEM]->TestValue(memory, sysmon_);
    }

    if (entity_alarm_.IsSet()) {
      if (CheckMonStatus() && entity_alarm_.TryClear()) {
         sysmon_->SendSMS(entity_alarm_.ToString()); // record clearing of alarm
         Schedule(timeout_s_);
      }
    } else {
      Schedule(timeout_s_); // reset timeout interval
      if (CheckMonStatus() == false) { // we are going into fault state
        entity_alarm_.TrySet("data monitoring alert");
        sysmon_->SendSMS(entity_alarm_.ToString());
        Cancel(); // clear timeout processing since we are now in failed state
        this->Recover(); // initiate recovery actions
      }
    }

  } else {
    // try generic entity set status message
    Entity::ProcessMessage(msg);
  }
}

void TaskEntity::Recover() {
  std::string cmd = "task-restart ";
  cmd += name_;
  sysmon_->ExecuteAction(cmd.c_str());
  // TODO(bsuter): implement full cascading recovery
}
