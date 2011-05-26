// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LIB_FM_DEVICE_MONITOR_H__
#define LIB_FM_DEVICE_MONITOR_H__

#include "lib/fm/constants.h"
#include "lib/util/stopwatch.h"

// Fault management client for a hardware peripheral device
// (sensor or actuator) which is expected to produce or
// consume a periodic stream of I/O events.
class DeviceMonitor {
 public:

  DeviceMonitor(const char *name, long timeout_ms);

  // For each iteration report if the operations was
  // a success or failure. Iterations which result in
  // hanging read or read timeouts should not be reported
  // as data error (device will eventually be timed out).
  void Ok();
  void CommError(); // e.g. CRC errors, msg corruption etc.
  void DevError(); // unexpected errors from the device

  // Explicitly report task status to system monitor (optional)
  void SetStatus(FM_STATUS status, const char *msg);

 private:
  void Keepalive(bool force = false);

  StopWatch timer_;
  long timeout_ms_;
  char device_name_[30];
  long valid_count_;
  long comm_err_count_;
  long dev_err_count_;
};

#endif // LIB_FM_LOG_H__
