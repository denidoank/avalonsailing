// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/util/delayed_event.h"

void DelayedEvent::Schedule(long delay_s) {
  Cancel();
  deadline_s_ = time(NULL) + delay_s;

  // A bit crude - if current timeslot is taken, try the next one.
  while (schedule_.find(deadline_s_) != schedule_.end()) {
    deadline_s_++;
  }
  schedule_[deadline_s_] = this;
}

void DelayedEvent::Cancel() {
  std::map<time_t, DelayedEvent *>::iterator it = schedule_.find(deadline_s_);
  if (it != schedule_.end()) {
    schedule_.erase(it);
  }
}

void DelayedEvent::Dispatch() {
  time_t now = time(NULL);

  while (schedule_.begin() != schedule_.end() &&
         schedule_.begin()->first <= now) {
    DelayedEvent *event = schedule_.begin()->second;
    schedule_.erase(schedule_.begin());
    event->Handle();
  }
}

long DelayedEvent::GetWaitTimeS(long default_delay_s) {
  time_t now = time(NULL);

  if (schedule_.begin() == schedule_.end()) {
    return default_delay_s;
  }
  long delay = schedule_.begin()->first - now;
  return (delay > 0) ? delay : 0;
}

std::map<time_t, DelayedEvent *> DelayedEvent::schedule_;
