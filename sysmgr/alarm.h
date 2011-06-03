// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SYSMGR_ALARM_H__
#define SYSMGR_ALARM_H__

#include <string>

#include "io/ipc/key_value_pairs.h"
#include "lib/util/timestamp.h"

// Representation of an alarm state.
// Alarms are set immediately once the alarm condition is encountered,
// but requires the condition to have not occured any more for
// at least a certain amount hold time.
// Hold time starts with a configured time period and is increased by 10%
// with each new re-occurence of the same alarm in order to reduce
// flapping even over larger time-scales.
class Alarm {
 public:
  Alarm(const char *owner, const char *description, int hold_time_s);

  const Timestamp &TriggerTime() const { return set_time_; };
  const Timestamp &LastHoldTime() const { return hold_start_time_; }
  int FlapCount() const { return flap_count_; };

  // True if the alarm is currently set.
  bool IsSet() const { return state_ == SET || state_ == HOLD;}

  // True if the underlying fault condition is currently met.
  bool IsFault() const { return state_ == SET; }

  // Returns true if the alarm did change state from this call.
  // The return value indicating a state transition of the alarm can
  // typically be used to trigger telemetry and/or recovery actions.
  bool Update(bool fault_condition);

  // Returns true, if this call did toggle the alarm into SET state.
  bool TrySet(const char *cause = "");

  // Returns true, if this call did toggle the alarm into CLEAR state.
  bool TryClear() { return Update(false); }

  // String formatting for telemetry messages etc.
  std::string ToString() const;

  // Load essential state from persistent store
  void LoadState(const KeyValuePair &data);

  // Store essential state to persistent store
  void StoreState(KeyValuePair *data);

 private:
  enum State {
    CLEAR,
    SET,
    HOLD
  };
  State state_;
  Timestamp set_time_; // Timestamp of when the alarm is set
  Timestamp hold_start_time_; // Timestamp of the beginning of a hold period
  int alarm_count_;
  int flap_count_; // Number of fault condition flaps during hold state.
  std::string cause_;

  const std::string owner_;
  const std::string description_;
  const int hold_interval_s_;
};

#endif // SYSMGR_ALARM_H__
