// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SYSMGR_MON_VAR_H__
#define SYSMGR_MON_VAR_H__

#include <math.h>
#include <string>

#include "lib/filter/sliding_average_filter.h"
#include "sysmgr/alarm.h"


class SysMon;
// Alarmed variable monitor.
//
// Triggers an alarm, if the associated variable value exceeds the threshold,
// which can be either upper or lower bound.
// Values are filtered through a sliding-window average before the threshold
// check.
class MonVar {
 public:
  MonVar(double threshold, int filter_window, bool upper_bound,
         long hold_down_s, const char *name,
         const char *desc) : threshold_(threshold), upper_bound_(upper_bound),
                             filter_(filter_window),
                             var_alarm_(name, desc, hold_down_s), name_(name),
                             min_(HUGE_VAL), max_(-HUGE_VAL),
                             last_(threshold) {};

  // Update the value of the variable and trigger alarm set/clear
  // if appropriate and send telemetry messages on alarm transitions.
  bool TestValue(double value, SysMon *sysmon);

  // Check the current alarm status of this variable monitor.
  bool CheckStatus() const;

  // String formatting for telemetry messages.
  std::string ToString() const;

 private:
  void ManageAlarm(bool condition, SysMon *sysmon);

  SlidingAverageFilter filter_;
  double threshold_;
  double last_;
  double min_;
  double max_;
  Alarm var_alarm_;
  const bool upper_bound_;
  const std::string name_;
};

#endif // SYSMGR_MON_VAR_H__
