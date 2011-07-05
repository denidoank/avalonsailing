// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/passive_entity.h"

#include "sysmgr/sysmon.h"

PowerBusEntity::~PowerBusEntity() {};

void PowerBusEntity::RecoveryAction() {
  sysmon_->SendSMS((std::string("power-cycle ") + name_).c_str());
  sysmon_->ExecuteAction(cmd_.c_str());
}

void SystemEntity::RecoveryAction() {
  sysmon_->StoreAlarmLog();
  Timestamp now;
  sysmon_->SendSMS(now.ToIsoString() + " rebooting now...");
  sysmon_->ExecuteAction("keepalive");
  sleep(10); // give sat-com module some time to send last message
  PowerBusEntity::RecoveryAction();
}
