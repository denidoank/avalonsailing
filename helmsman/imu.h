// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
// Derived from original imu.h file.

#ifndef HELMSMAN_IMU_H
#define HELMSMAN_IMU_H

// Original units from IMU, Controllers needs metric units and radians anyway. 
struct Imu {
  void Reset() {
    speed_m_s = 0;
    position.longitude_deg = 0;
    position.latitude_deg = 0;
    position.altitude_deg = 0;
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
  
  double speed_m_s; // in m/s
  // GPS-Data
  struct Position {
    double longitude_deg;
    double latitude_deg;
    double altitude_deg;
  } position;

  // IMU-Data
  struct Attitude {
    double phi_x_rad;   // roll;
    double phi_y_rad;   // pitch;
    double phi_z_rad;   // yaw;
  } attitude;
  struct Velocity {
    double x_m_s;
    double y_m_s;
    double z_m_s;
  } velocity;           // velocity in x, y, z in m/s
  struct Acceleration {
    double x_m_s2;
    double y_m_s2;
    double z_m_s2;
  } acceleration;       // acceleration in x, y, z in m/s^2
  struct Giro {
    double omega_x_rad_s;
    double omega_y_rad_s;
    double omega_z_rad_s;
  } gyro;               // gyroscope data in x, y, z in rad/s
  double temperature_c; // in deg C        
};

#endif  // HELMSMAN_IMU_H
