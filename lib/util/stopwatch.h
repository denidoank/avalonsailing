// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LOG_UTIL_STOPWATCH_H__
#define LOG_UTIL_STOPWATCH_H__

#include <stddef.h>
#include <sys/time.h>

// Utility class to keep track of elapsed real-time in ms
// since the last set.
class StopWatch {
 public:
  StopWatch() { Set(); }
  void Set() {
    gettimeofday(&start_time_, NULL);
  }
  // Return elapsed time in milliseconds
  long Elapsed() const {
    timeval current_time;
    gettimeofday(&current_time, NULL);
    return static_cast<long>((current_time.tv_sec * 1000LL +
                              current_time.tv_usec / 1000LL) -
                             (start_time_.tv_sec * 1000LL +
                              start_time_.tv_usec / 1000LL));
  }
  // Returns the current time in microseconds since the Epoch.
  static long long GetTimestampMicros() {
    timeval current_time;
    gettimeofday(&current_time, NULL);
    return current_time.tv_sec * 1000000LL + current_time.tv_usec;
  }

 private:
  timeval start_time_;
};

#endif // LOG_UTIL_STOPWATCH_H__
