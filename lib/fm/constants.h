// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LOG_FM_CONSTANTS_H__
#define LOG_FM_CONSTANTS_H__

enum FM_LOG_LEVELS {
  FM_LEVEL_FATAL = 0,
  FM_LEVEL_ERROR,
  FM_LEVEL_WARNING,
  FM_LEVEL_INFO,
  FM_LEVEL_DEBUG,
  FM_LEVEL_MAX // do not use
};
// Keep in sync with FM_LOG_LEVELS enum.
#define FM_LEVEL_NAMES_ {'F', 'E', 'W', 'I', 'D'}
#define FM_LEVEL_SYSLOG_MAP_ {LOG_CRIT, LOG_ERR, LOG_WARNING, LOG_INFO, LOG_DEBUG}

// Status of a monitored entity (process or sensor)
enum FM_STATUS {
  FM_STATUS_OK = 0, // fully working
  FM_STATUS_DEGRADED, // e.g. high error rate from a sensor
  FM_STATUS_FAULT // no longer able to provide any (partially) useful service
};
// Keep in sync with FM_STATUS enum.
#define FM_STATUS_NAMES_ {"OK", "DEGRADED", "FAULT"}

#endif //  LOG_FM_CONSTANTS_H__
