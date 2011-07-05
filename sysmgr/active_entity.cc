// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/active_entity.h"

#include <stdio.h>
#include <string.h>

#include "lib/fm/fm_messages.h"
#include "lib/fm/log.h"
#include "sysmgr/sysmon.h"

ActiveEntity::~ActiveEntity() {
 for (MonVarList::iterator it = mon_var_list_.begin();
       it != mon_var_list_.end(); ++it) {
    delete *it;
  }
}

void ActiveEntity::Handle() {
  if (entity_alarm_.IsSet()) {
    this->HandleRecovery();
  } else {
    if (entity_alarm_.TrySet("timeout")) {
      sysmon_->SendSMS(entity_alarm_.ToString());
      recovery_index_ = 0;
      this->HandleRecovery();
    }
  }
}

void ActiveEntity::HandleRecovery() {
  long wait_time;
  if (escalation_order_[recovery_index_]->TriggerRecovery(entity_alarm_,
                                                          &wait_time)) {
    FM_LOG_INFO("%s - recovery escalation to %s", name_.c_str(),
                escalation_order_[recovery_index_]->GetName().c_str());
    if (recovery_index_ < escalation_order_.size() - 1) {
      recovery_index_++;
    }
  }
  FM_LOG_INFO("%s - wait %lds for next recovery stage", name_.c_str(), wait_time);
  Schedule(wait_time);
}

void ActiveEntity::ProcessMessage(const KeyValuePair &msg) {
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
        HandleRecovery();
      }
    } else { // currently in FAULT state
      if (status == "OK"
          && CheckMonStatus()
          && entity_alarm_.TryClear()) {
        sysmon_->SendSMS(entity_alarm_.ToString()); // record clearing of alarm
      }
    }
    HandleKeepalive();
  } else {
    FM_LOG_ERROR("%s - unsupported msg event: '%s'", name_.c_str(),
                 msg.ToString(false).c_str());
  }
}

void ActiveEntity::HandleKeepalive() {
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
      HandleRecovery(); // initiate recovery actions
    }
  }
}

bool ActiveEntity::CheckMonStatus() {
  bool status = true;
  for (MonVarList::iterator it = mon_var_list_.begin();
       it != mon_var_list_.end(); ++it) {
    status &= (*it)->CheckStatus();
  }
  return status;
}

void ActiveEntity::LoadState(const KeyValuePair &data) {
  entity_alarm_.LoadState(data);
  Entity::LoadState(data);
}

void ActiveEntity::StoreState(KeyValuePair *data) {
  Entity::StoreState(data);
  recovery_policy_.StoreState(data);
}



TaskEntity::TaskEntity(const char *name, int timeout_s, int hold_time_s,
                       const RecoveryPolicy &policy, const EntityList &escalation,
                       SysMon *sysmon, const Properties &prop) :
    ActiveEntity(name, timeout_s, hold_time_s, policy, escalation, sysmon) {
  mon_var_list_.resize(VAR_MEM + 1);

  // put ourselves (local recovery) in the first slot of the recovery order
  escalation_order_.insert(escalation_order_.begin(), this);

  std::string ent = name;

  mon_var_list_[VAR_ERRORS] = new MonVar(prop.Get(ent + ".ERROR.th", 0.5),
                                         prop.Get(ent + ".ERROR.win", 10L),
                                         true, 2 * timeout_s,
                                         "ERROR", "rate of error log messages");

  mon_var_list_[VAR_LOG] = new MonVar(prop.Get(ent + ".LOG.th", 2.0),
                                      prop.Get(ent + ".LOG.win", 10L),
                                      true, 2 * timeout_s,
                                      "LOG", "rate of all log messages");

  mon_var_list_[VAR_CPU] = new MonVar(prop.Get(ent + ".CPU.th", 0.8),
                                      prop.Get(ent + ".CPU.win", 10L),
                                      true, 2 * timeout_s,
                                      "CPU", "avg usage");

  mon_var_list_[VAR_MEM] = new MonVar(prop.Get(ent + ".MEM.th",
                                               1024 * 1024 * 10.0),
                                      prop.Get(ent + ".MEM.win", 3L),
                                      true,
                                      2 * timeout_s,
                                      "MEM", "usage");
}

TaskEntity::~TaskEntity() {}

void TaskEntity::ProcessMessage(const KeyValuePair &msg) {
  bool extracted = true;
  long time_ms, errors, warnings, info, cpu, memory;

  extracted &= msg.GetLong("time", &time_ms);
  extracted &= msg.GetLong("err", &errors);
  extracted &= msg.GetLong("warn", &warnings);
  extracted &= msg.GetLong("info", &info);
  extracted &= msg.GetLong("cpu", &cpu);
  extracted &= msg.GetLong("mem", &memory);

  if (extracted) {
    if (time_ms > 0) {
      mon_var_list_[VAR_ERRORS]->TestValue(1000 * errors / time_ms,
                                           sysmon_);
      mon_var_list_[VAR_LOG]->TestValue(1000 * (errors + warnings + info) /
                                        time_ms, sysmon_);
      mon_var_list_[VAR_CPU]->TestValue(1000 * cpu / time_ms, sysmon_);
      mon_var_list_[VAR_MEM]->TestValue(memory, sysmon_);
    }
    HandleKeepalive();
  } else {
    // try generic entity set status message
    ActiveEntity::ProcessMessage(msg);
  }
}

void TaskEntity::RecoveryAction() {
  std::string cmd = "task-restart ";
  cmd += name_;
  sysmon_->ExecuteAction(cmd.c_str());
}



DeviceEntity::DeviceEntity(const char *name, int timeout_s, int hold_time_s,
                           const RecoveryPolicy &policy, const EntityList &escalation,
                           SysMon *sysmon, const Properties &prop) :
    ActiveEntity(name, timeout_s, hold_time_s, policy, escalation, sysmon) {
  mon_var_list_.resize(VAR_DEV_ERR + 1);

  std::string ent = name;
  long var_alarm_hold_s = prop.Get(ent + "var_alarm_hold_time",
                                   static_cast<long>(timeout_s));

  mon_var_list_[VAR_OK] = new MonVar(prop.Get(ent + ".OK.th", 0.5),
                                     prop.Get(ent + ".OK.win", 3L),
                                     false, var_alarm_hold_s, "OK",
                                     "rate of successful operations");

  mon_var_list_[VAR_COMM_ERR] = new MonVar(prop.Get(ent + ".CERR.th", 2.0),
                                           prop.Get(ent + ".CERR.win", 3L),
                                           true, var_alarm_hold_s, "CERR",
                                           "rate of communication errors");

  mon_var_list_[VAR_DEV_ERR] = new MonVar(prop.Get(ent + ".DERR.th", 0.8),
                                          prop.Get(ent + ".DERR.win", 3L),
                                          true, var_alarm_hold_s, "DERR",
                                          "rate of device errors");
}

DeviceEntity::~DeviceEntity() {}

void DeviceEntity::ProcessMessage(const KeyValuePair &msg) {
  bool extracted = true;
  long time_ms, ok, cerr, derr;

  extracted &= msg.GetLong("time", &time_ms);
  extracted &= msg.GetLong("valid", &ok);
  extracted &= msg.GetLong("cerr", &cerr);
  extracted &= msg.GetLong("hwerr", &derr);

  if (extracted) {
    if (time_ms > 0) {
      mon_var_list_[VAR_OK]->TestValue(1000.0 * ok / time_ms,
                                           sysmon_);
      mon_var_list_[VAR_COMM_ERR]->TestValue(1000.0 * cerr /
                                        time_ms, sysmon_);
      mon_var_list_[VAR_DEV_ERR]->TestValue(1000.0 * derr / time_ms, sysmon_);

    }
    HandleKeepalive();
  } else {
    // try generic entity set status message
    ActiveEntity::ProcessMessage(msg);
  }
}

void DeviceEntity::RecoveryAction() {
  FM_LOG_ERROR("No local recovery action for DeviceEntity %s",  name_.c_str());
}
