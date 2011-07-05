// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/entity.h"

#include <stdio.h>
#include <string.h>

#include "lib/fm/fm_messages.h"
#include "lib/fm/log.h"
#include "sysmgr/sysmon.h"

Entity::~Entity() { }

void Entity::ProcessMessage(const KeyValuePair &msg) {
  FM_LOG_ERROR("%s does not support monitoring messages", name_.c_str());
  }

bool Entity::TriggerRecovery(const Alarm &root_cause, long *wait_time) {
  *wait_time = recovery_policy_.NextAuthorizedRecoveryTimeS(root_cause);
  if (*wait_time > 0) {
    return false;
  }
  recovery_policy_.StartRecovery(root_cause);
  this->RecoveryAction();
  *wait_time = recovery_policy_.GetRepairTimeS();
  return true;
}

void Entity::LoadState(const KeyValuePair &data) {
  recovery_policy_.LoadState(data);
}

void Entity::StoreState(KeyValuePair *data) {
  data->Add("entity", name_);
  recovery_policy_.StoreState(data);
}
