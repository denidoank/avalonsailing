// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "modem/status.h"
#include "lib/testing/testing.h"
#include "lib/testing/pass_fail.h"
#include "proto/imu.h"
#include "proto/modem.h"
#include "proto/wind.h"

#include <string>

using namespace std;

int main(int argc, char *argv[]) {
  IMUProto imu_status = INIT_IMUPROTO;
  WindProto wind_status = INIT_WINDPROTO;
  ModemProto modem_status = INIT_MODEMPROTO;

  time_t status_time = 1314713716;  // 30.08.2011 14:15:16

  // Timestamp status only.
  EXPECT_EQ(BuildStatusMessage(status_time, imu_status, wind_status,
                               modem_status, "status-failed"),
            "30141516XXXXXXXXXXXXXXXXXXXXXXXXXNN00000status-failed");

  imu_status.timestamp_ms = 0;  // Invalid
  imu_status.lat_deg = 45.123;  // N45.07.22
  imu_status.lng_deg = -8.789;  // W008.47.20

  wind_status.timestamp_ms = (status_time - 1) * 1000;  // 1 second ago.
  // TODO(mariusv): Update wind direction and speed from helmsmann
  wind_status.angle_deg = 123.55;  // Average angle 120 degrees
  wind_status.speed_m_s = 11.88;   // 11.88 mps = 23 knots

  modem_status.coarse_position_lat = 45.001;  // N45.00.03
  modem_status.coarse_position_lng = -8.999;  // W008.59.56
  modem_status.position_timestamp_s = status_time - 3600;  // 1h ago.

  string status = BuildStatusMessage(status_time, imu_status, wind_status,
                                     modem_status, "status-ok");

  // Status using Iridium based position.
  EXPECT_EQ(BuildStatusMessage(status_time, imu_status, wind_status,
                               modem_status, "status-ok"),
            "30141516IN450003W0085956XXXXX1223NN00000status-ok");

  // Status using IMU based GPS position.
  imu_status.timestamp_ms = (status_time - 1) * 1000;  // 1 second ago.
  EXPECT_EQ(BuildStatusMessage(status_time, imu_status, wind_status,
                               modem_status, "status-ok"),
            "30141516GN450722W0084720XXXXX1223NN00000status-ok");

  PF_TEST(true, "Building SMS status.");
}
