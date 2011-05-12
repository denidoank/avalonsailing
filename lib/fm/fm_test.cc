// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/fm/fm.h"
#include "lib/fm/device_monitor.h"
#include "lib/fm/log.h"
#include "lib/testing/pass_fail.h"

// Run as a manual test with different FM commandline options to test
// integration with other system components (syslogd, symon).
//
// For minimal interference run
// fm_test --logtostderr --no-syslog --sysmon-socket /dev/null

int main(int argc, char **argv) {
  FM::Init(argc, argv);

  StopWatch timer;
  FM_LOG_ERROR("test of error logging");
  FM_LOG_WARN("test of warning logging");
  FM_LOG_INFO("test of info logging");
  FM_LOG_DEBUG("test of debug logging");
  FM_LOG_PERROR("test of system call error logging");

  FM_LOG_INFO("test with %s number of parameters: %d %f %ld",
           "variable", 1, 2.0, 3L);
  FM_LOG_INFO("execution time: %ldms", timer.Elapsed());
  FM::Keepalive();

  DeviceMonitor test_device("test", 100);
  test_device.DataError();
  test_device.DataValid();
  test_device.SetStatus(FM_STATUS_OK, "set status to ok");

  PF_TEST(true, "did not crash until here...");
  FM_ASSERT(true);
  FM_ASSERT(false); // assertion should fail and process crash
}
