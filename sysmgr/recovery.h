// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SYSMGR_RECOVERY_H__
#define SYSMGR_RECOVERY_H__

#include <string>

#include <io/ipc/key_value_pairs.h>
#include <lib/util/timestamp.h>
#include <sysmgr/alarm.h>

// Tracks the timing of recovery actions for an entity.
//
// Timing configuration includes:
// - repair time: expected time for the recovery action to take effect.
//                Do not trigger again during this time for any reason.
// - retry time: time to wait before trying the same action again for the
//               same fault condition (typically very long, as this covers
//               non-repairable faults).
// We assume that if the recovery timestamp is after the alarm set-time, then
// the recovery action should count as a recovery attempt for this alarm
// condition.
class RecoveryPolicy {
 public:

  RecoveryPolicy(int repair_time_s,
                 int retry_time_s) : repair_time_s_(repair_time_s),
                                     retry_time_s_(retry_time_s) {};

  // Has this recovery been triggered after the alarm was set?
  bool IsRecovering(const Alarm &alarm) const;

  // Has this recovery been triggered less than the expected repair time ago?
  bool IsRepair(const Alarm &alarm) const;

  // Try to trigger this recovery for this alarm.
  // Returns true, if a new recovery attempt is authorized by the policy
  // for this alarm.
  bool StartRecovery(const Alarm &alarm);

  // Delay in seconds until we are allowed to start a recovery for this alarm.
  long NextAuthorizedRecoveryTimeS(const Alarm &alarm) const;

  long GetRepairTimeS() { return repair_time_s_; };

  void LoadState(const KeyValuePair &data);
  void StoreState(KeyValuePair *data) const;

 private:
  Timestamp last_recovery_; // defaults to now at construction

  const int repair_time_s_;
  const int retry_time_s_;
};

#endif // SYSMON_RECOVERY_H__
