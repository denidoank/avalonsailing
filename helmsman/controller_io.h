// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_CONTROLLER_IO_H
#define HELMSMAN_CONTROLLER_IO_H

#include "helmsman/imu.h"
#include "helmsman/wind.h"
#include "helmsman/drive_data.h"
#include "helmsman/skipper_input.h"

// Input definition
struct ControllerInput {
  void Reset () {
    imu.Reset();
    wind.Reset();
    drives.Reset();
    alpha_star = 0;  // natural
  }

  Imu imu;
  WindSensor wind;
  DriveActualValuesRad drives;  // Actual positions and Ready flags
  double alpha_star;            // from Skipper
};

// Output definition
struct ControllerOutput {
  void Reset() {
    drives_reference.Reset();
    skipper_input.Reset();
  }
  SkipperInput skipper_input;
  DriveReferenceValuesRad drives_reference;
};

struct FilteredMeasurements {
  void Reset() {
    phi_z_boat = 0;
    mag_boat = 0;
    omega_boat = 0;
    alpha_true = 0;
    mag_true = 0;
    angle_app = 0;
    mag_app = 0;
    angle_sensor = 0;
    mag_sensor = 0;
    angle_sail = 0;
    mag_sail = 0;
    longitude_deg = 0;
    latitude_deg = 0;
    phi_x_rad = 0;
    phi_y_rad = 0;
    temperature_c = 0;
    valid = false;
  }
  // Applied filters (T1 roughly 1s) to all data but the true wind direction.
  // true speed, global frame
  double phi_z_boat;
  double mag_boat;
  double omega_boat;
  // true wind direction, global frame, filtered strongly to be rather stable
  double alpha_true;
  double mag_true;
  // apparent wind angle, boat frame
  double angle_app;
  double mag_app;
  // sensor wind angle, mast frame
  double angle_sensor;
  double mag_sensor;
  // sail wind angle, is the negative angle of attack of the wind to the sail 
  double angle_sail;
  double mag_sail;
  // GPS data
  double longitude_deg;
  double latitude_deg;

  double phi_x_rad;     // roll or heel
  double phi_y_rad;     // pitch
  double temperature_c; // in deg C
  
  bool valid;           // All the filters have been filled up and contain
                        // reliable data. 
};

#endif  // HELMSMAN_CONTROLLER_IO_H
