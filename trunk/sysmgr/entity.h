// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SYSMGR_ENTITY_H__
#define SYSMGR_ENTITY_H__

#include <string>
#include <vector>

#include "io/ipc/key_value_pairs.h"
#include "lib/util/delayed_event.h"
#include "sysmgr/alarm.h"
#include "sysmgr/mon_var.h"

typedef std::vector<MonVar *> MonVarList;

class SysMon;

// Represents an abstract monitored entity.
//
// Primary active entities are tasks (unix procs) and hw devices.
// Each primary entity maps 1:1 to a client-side representation and
// processes the status messages it receives from it.
//
// Entities have containment/fate-sharing relationships with each other and
// each entity owns a particular recovery action. There are also passive
// entities, which do not receive any messages from a client, but only
// participate in recovery actions (main computer, edgeport serial converter,
// power buses etc.).
class Entity : public DelayedEvent {
 public:

  Entity(const char *name, int timeout_s,
         SysMon *sysmon) : entity_alarm_(name, "status",
                                         timeout_s),
                           timeout_s_(timeout_s),
                           sysmon_(sysmon),
                           name_(name) {Schedule(timeout_s);};
  virtual ~Entity();

  // Process a monitoring status message from FM client.
  virtual void ProcessMessage(const KeyValuePair &msg);

  // Trigger next phase of recovery (if alarm is still active).
  virtual void Recover() = 0;

  // Timer callback.
  void Handle();

  // Store and recover essential entity state from alarm/recovery log.
  void LoadState(const KeyValuePair &data);
  void StoreState(KeyValuePair *data);

 protected:
  bool CheckMonStatus();

  Alarm entity_alarm_; // Status alarm

  MonVarList mon_var_list_; // List of monitored variables

  const int timeout_s_; // Entity keepalive timeout
  const std::string name_;
  SysMon *sysmon_; // Interface to telemetry SMS & recovery actions
};

// Active Entity representing a unix process.
//
// Handles messages from FM:Keepalive and FM:SetStatus.
class TaskEntity : public Entity {
 public:
  TaskEntity(const char *name, int timeout_s, SysMon *sysmon);
  ~TaskEntity();

  void ProcessMessage(const KeyValuePair &msg);
  void Recover();
 private:
  // Variables reported in keepalive messages.
  enum {
    VAR_ERRORS, // Rate of log messages of severity error.
    VAR_LOG, // Overall rate of log messages.
    VAR_CPU, // CPU utilization.
    VAR_MEM // Memory allocation.
  };
};

// TODO(bsuter): implement DeviceEntity
// TODO(bsuter): implement passive entities for recovery
// corresponding to each reboot or power-cycle action.
#endif // SYSMGR_ENTITY_H__
