// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_CONTROLLER_IO_H
#define HELMSMAN_CONTROLLER_IO_H

#include <string.h>

#include "lib/unknown.h"
#include "lib/now.h"
#include "imu.h"
#include "compass_sensor.h"
#include "drive_data.h"
#include "wind_sensor.h"

#include "proto/compass.h"
#include "proto/imu.h"
#include "proto/rudder.h"
#include "proto/wind.h"
#include "proto/helmsman.h"


// Input definition
struct ControllerInput {
  ControllerInput() {
    Reset();
  }
  void Reset () {
    imu.Reset();
    wind_sensor.Reset();
    drives.Reset();
    compass_sensor.Reset();
    alpha_star_rad = kUnknown;  // natural
  }
  void ToProto(WindProto* wind_sensor_proto,
               RudderProto* status,
               IMUProto* imu_proto,
               CompassProto* compass_proto) {
    imu.ToProto(imu_proto);
    wind_sensor.ToProto(wind_sensor_proto);
    drives.ToProto(status, now_ms());
    compass_sensor.ToProto(compass_proto);
  }

  Imu imu;
  WindSensor wind_sensor;
  DriveActualValuesRad drives;  // Actual positions and Ready flags
  CompassSensor compass_sensor; // Compass device bearing
  double alpha_star_rad;        // from Skipper
};

// Output definition
struct ControllerOutput {
  void Reset() {
    memset(&skipper_input, 0, sizeof skipper_input);
    drives_reference.Reset();
  }

  HelmsmanStsProto skipper_input;
  DriveReferenceValuesRad drives_reference;
};

// elements (except lat long) in internal units (rad, m, s)
struct FilteredMeasurements {
  // Applied filters (T1 roughly 1s) to some data.
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
  // Angle of attack relative to the boom axis (sensor wind angle, mast frame)
  // Positive angle-of-attack pushes left when the sail is straight.
  // Slow filter.
  double angle_aoa;
  double mag_aoa;
  // GPS data
  double longitude_deg;
  double latitude_deg;

  double phi_x_rad;     // roll or heel
  double phi_y_rad;     // pitch
  double temperature_c; // in deg C

  bool valid;           // All the filters (except the true wind filter) have been
                        // filled up and contain reliable data.
  bool valid_app_wind;  // apparent wind info is reliable and well filtered.
  bool valid_true_wind; // true wind info is reliable and well filtered.

  void Reset() { memset(this, 0, sizeof *this);  }
};

#endif  // HELMSMAN_CONTROLLER_IO_H
