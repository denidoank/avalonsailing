// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#include "imu.h"

#include <stdio.h>

Imu::Imu() {
  Reset();
}

void Imu::Reset() {
  speed_m_s = 0;
  position.longitude_deg = 0;
  position.latitude_deg = 0;
  position.altitude_m = 0;
  attitude.phi_x_rad = 0;
  attitude.phi_y_rad = 0;
  attitude.phi_z_rad = 0;
  velocity.x_m_s = 0;
  velocity.y_m_s = 0;
  velocity.z_m_s = 0;
  acceleration.x_m_s2 = 0;
  acceleration.y_m_s2 = 0;
  acceleration.z_m_s2 = 0;
  gyro.omega_x_rad_s = 0;
  gyro.omega_y_rad_s = 0;
  gyro.omega_z_rad_s = 0;
  temperature_c = 20;
}

std::string Imu::ToString() const {
  char line[17*25];
  int s = snprintf(line, sizeof line, "speed_m_s: %f lat_deg:%f lon_deg:%f alt_m:%f "
      "phi_x:%.4f phi_y:%.4f phi_z:%.4f "
      "v_x:%.4f v_y:%.4f v_z:%.4f "
      "acc_x:%.4f acc_y:%.4f acc_z:%.4f "
      "om_x:%.4f om_y:%.4f om_z:%.4f "
      "theta: %.3f"
      "\n",
      speed_m_s,
      position.longitude_deg, position.latitude_deg, position.altitude_m,
      attitude.phi_x_rad, attitude.phi_y_rad, attitude.phi_z_rad,
      velocity.x_m_s, velocity.y_m_s, velocity.z_m_s,
      acceleration.x_m_s2, acceleration.y_m_s2, acceleration.z_m_s2,
      gyro.omega_x_rad_s, gyro.omega_y_rad_s, gyro.omega_z_rad_s,
      temperature_c);
  return std::string(line, s);
}
