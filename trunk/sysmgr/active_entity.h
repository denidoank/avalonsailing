// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SYSMGR_ACTIVE_ENTITY_H__
#define SYSMGR_ACTIVE_ENTITY_H__

#include "sysmgr/entity.h"

#include "lib/config/properties.h"

// An actively monitored entity
//
// Each active entity must receive monitoring messages within
// a certain timeout value, otherwise it's assoicated alarm will trigger.
// Each active entity also has a set of monitoring variables which are
// updated from data in the monitoring message. If any variable exceeds
// its alarm threshold, the entity alarm is triggered as well.
//
// Each active entities typically has a list of other entites it depends
// on for recovery (fate-sharing relationships). Once the alarm is
// triggered, the entity enters recovery mode, where it tries to
// escalate the recovery to its list of associated entities in
// a defined order. Each entities recovery policy governs when
// its particular action can be triggered next.

class ActiveEntity : public Entity, public DelayedEvent {
 public:

  ActiveEntity(const char *name, int timeout_s, int hold_time_s,
         const RecoveryPolicy &policy, const EntityList &escalation,
         SysMon *sysmon) : entity_alarm_(name, "status",
                                         hold_time_s),
                           escalation_order_(escalation),
                           recovery_index_(0),
                           timeout_s_(timeout_s),
                           Entity(name, policy, sysmon) {Schedule(timeout_s);};
  virtual ~ActiveEntity();

  // Process a monitoring status message from FM client.
  virtual void ProcessMessage(const KeyValuePair &msg);

  // Timer callback.
  void Handle();


  // Store and recover essential entity state from alarm/recovery log.
  void LoadState(const KeyValuePair &data);
  void StoreState(KeyValuePair *data);

 protected:
  bool CheckMonStatus();
  // Trigger next phase of recovery (if alarm is still active).
  void HandleRecovery();
  // Keepalive & timeout management when receiving any message.
  void HandleKeepalive();

  Alarm entity_alarm_; // Status alarm

  int recovery_index_;
  EntityList escalation_order_;

  MonVarList mon_var_list_; // List of monitored variables

  const int timeout_s_; // Entity keepalive timeout
};

// Active Entity representing a unix process.
//
// Handles messages from FM:Keepalive and FM:SetStatus.
class TaskEntity : public ActiveEntity {
 public:
  TaskEntity(const char *name, int timeout_s, int hold_time_s,
             const RecoveryPolicy &policy, const EntityList &escalation,
             SysMon *sysmon, const Properties &prop);
  ~TaskEntity();

  virtual void ProcessMessage(const KeyValuePair &msg);
 private:
  void RecoveryAction();

  // Variables reported in keepalive messages.
  enum {
    VAR_ERRORS, // Rate of log messages of severity error.
    VAR_LOG, // Overall rate of log messages.
    VAR_CPU, // CPU utilization.
    VAR_MEM // Memory allocation.
  };
};


class DeviceEntity : public ActiveEntity {
 public:
  DeviceEntity(const char *name, int timeout_s, int hold_time_s,
             const RecoveryPolicy &policy, const EntityList &escalation,
               SysMon *sysmon, const Properties &prop);
  ~DeviceEntity();

  virtual void ProcessMessage(const KeyValuePair &msg);

 private:
  void RecoveryAction();

  enum {
    VAR_OK,
    VAR_COMM_ERR,
    VAR_DEV_ERR
  };
};


#endif // SYSMGR_ACTIVE_ENTITY_H__
