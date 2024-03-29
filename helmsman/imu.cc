// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#include "imu.h"

#include <stdio.h>
#include "common/convert.h"
#include "common/normalize.h"
#include "helmsman/compass.h"

Imu::Imu() {
  Reset();
}

void Imu::Reset() {
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
  compass.phi_z_rad = 0;
  compass.valid = false;
}

std::string Imu::ToString() const {
  char line[20*25];
  int s = snprintf(line, sizeof line, "lat_deg:%lf lon_deg:%lf alt_m:%lf "
      "phi_x:%.4lf phi_y:%.4lf phi_z:%.4lf "
      "v_x:%.4lf v_y:%.4lf v_z:%.4lf "
      "acc_x:%.4lf acc_y:%.4lf acc_z:%.4lf "
      "om_x:%.4lf om_y:%.4lf om_z:%.4lf "
      "theta: %.3lf "
      "valid:%d compass:%.4lf  "
      "\n",
      position.longitude_deg, position.latitude_deg, position.altitude_m,
      attitude.phi_x_rad, attitude.phi_y_rad, attitude.phi_z_rad,
      velocity.x_m_s, velocity.y_m_s, velocity.z_m_s,
      acceleration.x_m_s2, acceleration.y_m_s2, acceleration.z_m_s2,
      gyro.omega_x_rad_s, gyro.omega_y_rad_s, gyro.omega_z_rad_s,
      temperature_c,
      compass.valid ? 1 : 0, compass.phi_z_rad);
  return std::string(line, s);
}

void Imu::ToProto(IMUProto* imu_proto) const {
  imu_proto->temp_c = temperature_c;

  imu_proto->acc_x_m_s2 = acceleration.x_m_s2;
  imu_proto->acc_y_m_s2 = acceleration.y_m_s2;
  imu_proto->acc_z_m_s2 = acceleration.z_m_s2;

  imu_proto->gyr_x_rad_s = gyro.omega_x_rad_s ;
  imu_proto->gyr_y_rad_s = gyro.omega_y_rad_s ;
  imu_proto->gyr_z_rad_s = gyro.omega_z_rad_s ;

  imu_proto->roll_deg  = Rad2Deg(attitude.phi_x_rad);
  imu_proto->pitch_deg = Rad2Deg(attitude.phi_y_rad);
  imu_proto->yaw_deg   = NormalizeDeg(Rad2Deg(attitude.phi_z_rad));

  imu_proto->lat_deg = position.latitude_deg;
  imu_proto->lng_deg = position.longitude_deg;
  imu_proto->alt_m   = position.altitude_m;

   // The imucat converts the speed components into the boat coordinate frame.
  imu_proto->vel_x_m_s = velocity.x_m_s;
  imu_proto->vel_y_m_s = velocity.y_m_s;
  imu_proto->vel_z_m_s = velocity.z_m_s;

  BearingToMagnetic(attitude.phi_z_rad,
      &imu_proto->mag_x_au,
      &imu_proto->mag_y_au,
      &imu_proto->mag_z_au);
}

void Imu::FromProto(const IMUProto& imu_proto) {
  temperature_c = imu_proto.temp_c;

  acceleration.x_m_s2 = imu_proto.acc_x_m_s2;
  acceleration.y_m_s2 = imu_proto.acc_y_m_s2;
  acceleration.z_m_s2 = imu_proto.acc_z_m_s2;

  gyro.omega_x_rad_s = imu_proto.gyr_x_rad_s;
  gyro.omega_y_rad_s = imu_proto.gyr_y_rad_s;
  gyro.omega_z_rad_s = imu_proto.gyr_z_rad_s;

  attitude.phi_x_rad = Deg2Rad(imu_proto.roll_deg);
  attitude.phi_y_rad = Deg2Rad(imu_proto.pitch_deg);
  attitude.phi_z_rad = SymmetricRad(Deg2Rad(imu_proto.yaw_deg));

  position.latitude_deg  = imu_proto.lat_deg;
  position.longitude_deg = imu_proto.lng_deg;
  position.altitude_m    = imu_proto.alt_m;

  // x-Speed is boat forward speed.
  velocity.x_m_s = imu_proto.vel_x_m_s;
  velocity.y_m_s = imu_proto.vel_y_m_s;
  velocity.z_m_s = imu_proto.vel_z_m_s;

  compass.phi_z_rad = NAN;
  compass.valid = VectorsToBearing(acceleration.x_m_s2,
                                   acceleration.y_m_s2,
                                   acceleration.z_m_s2,
                                   imu_proto.mag_x_au,
                                   imu_proto.mag_y_au,
                                   imu_proto.mag_z_au,
                                   &compass.phi_z_rad);
}
