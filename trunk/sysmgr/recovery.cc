// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/recovery.h"

#define RECOVERY_TIME "recovery_time"

bool RecoveryPolicy::IsRecovering(const Alarm &alarm) const {
  return alarm.TriggerTime().SecondsSince(last_recovery_) < 0;
}

bool RecoveryPolicy::IsRepair(const Alarm &alarm) const {
  Timestamp now;
  return (IsRecovering(alarm) &&
          now.SecondsSince(last_recovery_) < repair_time_s_);
}

bool RecoveryPolicy::StartRecovery(const Alarm &alarm) {
  if (NextAuthorizedRecoveryTimeS(alarm) == 0) {
    Timestamp now;
    last_recovery_ = now;
    return true;
  }
  return false;
}

long RecoveryPolicy::NextAuthorizedRecoveryTimeS(const Alarm &alarm) const {
  Timestamp now;
  // "net" elapsed time since last recover - could be negative if
  // during the repair interval.
  long elapsed = now.SecondsSince(last_recovery_);

  if (IsRecovering(alarm)) {
    // Make sure we don't try more often than retry-time for an alarm
    // which is already covered by a previous recovery.
    return (elapsed >= retry_time_s_) ? 0 : (retry_time_s_ - elapsed);
  } else {
    // Wait at least repair-time before triggering again for any reason.
    return (elapsed >= repair_time_s_) ? 0 : (repair_time_s_ - elapsed);
  }
}

void RecoveryPolicy::LoadState(const KeyValuePair &data) {
   std::string timestamp;
  if (data.Get(RECOVERY_TIME, &timestamp)) {
    last_recovery_.FromIsoString(timestamp.c_str());
  }
}

void RecoveryPolicy::StoreState(KeyValuePair *data) const {
  data->Add(RECOVERY_TIME, last_recovery_.ToIsoString());
}
