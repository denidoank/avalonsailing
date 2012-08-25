// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, April 2012
#include "helmsman/imu.h"

#include <string.h>
#include "proto/imu.h"
#include "lib/testing/testing.h"


ATEST(IMUData, All) {
  const char line[] =
      "imu: timestamp_ms:1335727963376 gps_timestamp_ms:1335727963377 temp_c:28.000 "
      "acc_x_m_s2:0.000 acc_y_m_s2:0.000 acc_z_m_s2:0.000 "
      "gyr_x_rad_s:0.000 gyr_y_rad_s:0.000 gyr_z_rad_s:0.000 "
      "mag_x_au:22.786 mag_y_au:-0.000 mag_z_au:41.107 "
      "roll_deg:0.000 pitch_deg:0.000 yaw_deg:0.000 "
      "lat_deg:48.238906 lng_deg:-4.769800 alt_m:0.000 "
      "vel_x_m_s:-0.586 vel_y_m_s:0.000 vel_z_m_s:0.000\n";
  IMUProto imu_proto = INIT_IMUPROTO;
  int chars_read;
  EXPECT_EQ(21, sscanf(line, IFMT_IMUPROTO(&imu_proto, &chars_read)));
  Imu imu;
  imu.Reset();
  // fprintf(stderr, "Resetted: %s\n", imu.ToString().c_str());
  imu.FromProto(imu_proto);
  // fprintf(stderr, "FromProto: %s\n", imu.ToString().c_str());
  // The compass needs the gravity vector to get the horizontal plane.
  EXPECT_EQ(0, imu.compass.valid);
  EXPECT_TRUE(isnan(imu.compass.phi_z_rad));

  imu_proto.acc_z_m_s2 = -9.81;
  imu.FromProto(imu_proto);
  // fprintf(stderr, "FromProto: %s\n", imu.ToString().c_str());
  EXPECT_EQ(1, imu.compass.valid);
  EXPECT_EQ(0, imu.compass.phi_z_rad);

  imu_proto.mag_y_au = -22.786;
  imu.FromProto(imu_proto);
  // fprintf(stderr, "FromProto: %s\n", imu.ToString().c_str());
  EXPECT_EQ(1, imu.compass.valid);
  EXPECT_EQ(M_PI/4, imu.compass.phi_z_rad);

  // Modified timestamp, acc_z
  const char line2[] =
      "imu: timestamp_ms:0 gps_timestamp_ms:0 temp_c:28.000 "
      "acc_x_m_s2:0.000 acc_y_m_s2:0.000 acc_z_m_s2:-9.810 "
      "gyr_x_rad_s:0.000 gyr_y_rad_s:0.000 gyr_z_rad_s:0.000 "
      "mag_x_au:22.786 mag_y_au:-0.000 mag_z_au:41.107 "
      "roll_deg:0.000 pitch_deg:0.000 yaw_deg:0.000 "
      "lat_deg:48.2389060 lng_deg:-4.7698000 alt_m:0.000 "
      "vel_x_m_s:-0.586 vel_y_m_s:0.000 vel_z_m_s:0.000\n";
  IMUProto imu_proto_out = INIT_IMUPROTO;
  imu.ToProto(&imu_proto_out);
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), OFMT_IMUPROTO(imu_proto_out));
  // fprintf(stderr, "line2: %s\n", line2);
  // fprintf(stderr, "buffer: %s\n", buffer);

  EXPECT_TRUE(strcmp(line2, buffer) == 0);
}


int main(int argc, char* argv[]) {
  testing::RunAllTests();
  return 0;
}
