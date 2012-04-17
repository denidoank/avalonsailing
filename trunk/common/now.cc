// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.


#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>

#include <stdint.h>

#include <sys/time.h>
#include <syslog.h>
#include <time.h>

int64_t now_micros() {
  timeval tv;
  if (gettimeofday(&tv, NULL) < 0) {
    syslog(LOG_CRIT, "gettimeofday failed");
    return 0;
  }  
  return tv.tv_sec * 1000000LL + tv.tv_usec;
}

int64_t now_ms() {
  return now_micros() / 1000;
}

int64_t now_s() {
  timeval tv;
  if (gettimeofday(&tv, NULL) < 0) {
    syslog(LOG_CRIT, "gettimeofday failed");
    return 0;
  }  
  return tv.tv_sec;
}

