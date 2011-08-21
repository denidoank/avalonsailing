// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_CONTROLLER_IO_H
#define HELMSMAN_CONTROLLER_IO_H

#include "common/unknown.h"
#include "helmsman/imu.h"
#include "helmsman/wind.h"
#include "helmsman/drive_data.h"
#include "helmsman/skipper_input.h"
#include "helmsman/helmsman_status.h"
#include "proto/rudder.h"
#include "proto/wind.h"
#include "proto/imu.h"

// Input definition
struct ControllerInput {
  ControllerInput() {
    Reset();
  }  
  void Reset () {
    imu.Reset();
    wind_sensor.Reset();
    drives.Reset();
    alpha_star_rad = kUnknown;  // natural
  }
  void ToProto(WindProto* wind_sensor_proto,
               RudderProto* drive_actual_proto,
               IMUProto* imu_proto) {
    imu.ToProto(imu_proto);
    wind_sensor.ToProto(wind_sensor_proto);
    drives.ToProto(drive_actual_proto);
  }

  Imu imu;
  WindSensor wind_sensor;
  DriveActualValuesRad drives;  // Actual positions and Ready flags
  double alpha_star_rad;        // from Skipper
};

// Output definition
struct ControllerOutput {
  void Reset() {
    drives_reference.Reset();
    skipper_input.Reset();
    status.Reset();
  }
  bool operator!=(const ControllerOutput& r) {
    return skipper_input != r.skipper_input ||
           drives_reference != r.drives_reference;
  }  
  SkipperInput skipper_input;
  DriveReferenceValuesRad drives_reference;
  HelmsmanStatus status;
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
