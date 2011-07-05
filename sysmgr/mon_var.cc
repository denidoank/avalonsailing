// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/mon_var.h"

#include <algorithm>

#include "sysmgr/sysmon.h"

bool MonVar::TestValue(double value, SysMon *sysmon) {
  last_ = filter_.Filter(value);
  if (!filter_.ValidOutput()) {
    return false;
  }
  bool condition = upper_bound_ ? last_ > threshold_ : last_ < threshold_;
  min_ = std::min(min_, last_);
  max_ = std::max(max_, last_);
  ManageAlarm(condition, sysmon);
  return condition;
}

bool MonVar::CheckStatus() const {
  return var_alarm_.IsFault() == false;
}

std::string MonVar::ToString() const {
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "%s: %.3f%c%.3f [%.3f, %.3f]%s",
           name_.c_str(),
           last_, upper_bound_ ? '>' : '<', threshold_,
           min_, max_, var_alarm_.IsSet() ? "*" : "");
  return std::string(buffer);
}

void MonVar::ManageAlarm(bool condition, SysMon *sysmon) {
  if (var_alarm_.Update(condition)) {
    sysmon->SendSMS(var_alarm_.ToString());
  }
}
