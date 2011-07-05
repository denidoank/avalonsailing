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
#include "sysmgr/recovery.h"

class Entity;
class SysMon;

typedef std::vector<MonVar *> MonVarList;
typedef std::vector<Entity *> EntityList;

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

class Entity {
 public:
  Entity(const char *name, const RecoveryPolicy &policy, SysMon *sysmon) :
      name_(name),
      recovery_policy_(policy),
      sysmon_(sysmon) {};
  virtual ~Entity();
  virtual void ProcessMessage(const KeyValuePair &msg);

   // Store and recover essential entity state from alarm/recovery log.
  virtual void LoadState(const KeyValuePair &data);
  virtual void StoreState(KeyValuePair *data);

  // Checks recovery policy if it can execute recovery action
  // for this root-cause alarm. If yes, RecoveryAction is called
  // and the function returns true.
  // In case of succcess, wait_time is set to repair time of this recovery
  // action and otherwise to the necessary waiting time until the
  // recovery can be executed.
  bool TriggerRecovery(const Alarm &root_cause, long *wait_time);

  const std::string &GetName() const { return name_; }

 protected:
  // Hook for executing the appropriate local recovery action for this entity.
  // (Called by self-delegation from TriggerRecovery after policy check)
  virtual void RecoveryAction() = 0;

  const std::string name_;
  RecoveryPolicy recovery_policy_;
  SysMon *sysmon_;
};

#endif // SYSMGR_ENTITY_H__
