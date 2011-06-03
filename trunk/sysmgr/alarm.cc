// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/alarm.h"

#include <stdio.h>

#include "lib/fm/log.h"

// Keys for storing alarm properties alarm/recovery log
#define ALARM_TIME "alarm_time"
#define ALARM_CAUSE "alarm_cause"

Alarm::Alarm(const char *owner,
             const char *description,
             int hold_time_s) : state_(CLEAR),
                                alarm_count_(0),
                                owner_(owner), description_(description),
                                hold_interval_s_(hold_time_s) {
                                 }

bool Alarm::Update(bool fault_condition) {
  switch (state_) {
    case CLEAR:
      if (fault_condition) {
        Timestamp now;
        set_time_ = now;
        state_ = SET;
        return true;
      }
      break;
    case SET:
      if (!fault_condition) {
        Timestamp now;
        hold_start_time_ = now;
        state_ = HOLD;
      }
      // fall-through (case of zero hold interval)
    case HOLD:
      if (fault_condition) {
        state_ = SET; // Abandon hold state.
        flap_count_++;
      } else {
        Timestamp now;
        // If in hold long enough without re-occurence of the fault,
        // we can clear the alarm now.
        if (now.SecondsSince(hold_start_time_) >= hold_interval_s_) {
          state_ = CLEAR;
          return true;
        }
      }
  }
  return false;
}

bool Alarm::TrySet(const char *cause) {
  if (Update(true)) {
    alarm_count_++;
    cause_ = cause;
    return true;
  } else {
    return false;
  }
}

std::string Alarm::ToString() const {
  char buffer[100];
  if (IsSet()) {
    snprintf(buffer, sizeof(buffer), "%s SET(%d) %s/%s: %s",
             set_time_.ToIsoString().c_str(), alarm_count_,
             owner_.c_str(), description_.c_str(), cause_.c_str());
  } else {
    snprintf(buffer, sizeof(buffer), "%s CLEAR(%d) %s/%s",
             hold_start_time_.ToIsoString().c_str(), alarm_count_,
             owner_.c_str(), description_.c_str());
  }
  return std::string(buffer);
}

void Alarm::LoadState(const KeyValuePair &data) {
  std::string timestamp;
  if (data.Get(ALARM_TIME, &timestamp)) {
    state_ = SET;
    set_time_.FromIsoString(timestamp.c_str());
    data.Get(ALARM_CAUSE, &cause_);
    FM_LOG_INFO("reloaded alarm %s - %s %s", owner_.c_str(),
             timestamp.c_str(), cause_.c_str());
  }
}

void Alarm::StoreState(KeyValuePair *data) {
  if (IsSet()) {
    data->Add(ALARM_TIME, set_time_.ToIsoString());
    data->Add(ALARM_CAUSE, cause_);
  }
}
