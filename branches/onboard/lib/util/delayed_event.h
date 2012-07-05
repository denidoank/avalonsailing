// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LIB_UTIL_DELAYED_EVENT_H__
#define LIB_UTIL_DELAYED_EVENT_H__

#include <map>

#include <time.h>

// Very simple scheduling facility to trigger an event
// by some time-delta into the future.
//
// No guarantee that the event will be called in N seconds,
// just not before that.
//
// This class uses static data and is not thread-safe.
class DelayedEvent {
 public:
  DelayedEvent() : deadline_s_(0) {};

  // Function to be called
  virtual void Handle() = 0;

  // Schedule this event at least delay_s seconds into the future.
  void Schedule(long delay_s);

  // Cancel this event, if it has been scheduled.
  void Cancel();

  // Call Handle() on the scheduled events which are currently expired.
  static void Dispatch();

  // Return the time until the first scheduled event expires or the
  // default delay if there are no scheduled events.
  // This is typically used to configure sleep or I/O timeout.
  static long GetWaitTimeS(long default_delay_s);

 private:
  time_t deadline_s_;

  static std::map<time_t, DelayedEvent *> schedule_;
};
#endif // LIB_UTIL_DELAYED_EVENT_H__
