// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

// Fill internal data structures from raw input strings

#ifndef HELMSMAN_FILL_H
#define HELMSMAN_FILL_H

#include <string>

#include "helmsman/imu.h"
#include "helmsman/wind.h"
#include "helmsman/drive_data.h"

using namespace std;

bool FillImu(const string& raw, 
             Imu* imu);

bool FillWind(const string& raw, 
              WindRad* wind_rad);

bool FillDrivesActual(const string& raw, 
                      DriveActualValuesRad* drive_actual_rad);

/* for refernce
// All values in degrees. For the drives the [-180, 180] convention is used.
// The drive interface handles the wrap around from -179 to 179 degrees.
struct DriveReferenceValues {
  DriveReferenceValues();
  DriveReferenceValues(const DriveReferenceValuesRad& ref_rad);
  void Reset();
  void Check();
  double gamma_rudder_star_left_deg;
  double gamma_rudder_star_right_deg;
  double gamma_sail_star_deg;
};

// Internally used radians versions.
struct DriveActualValuesRad {
  DriveActualValuesRad();
  DriveActualValuesRad(const DriveActualValues& act_deg);
  void Reset();
  double gamma_rudder_left_rad;
  double gamma_rudder_right_rad;
  double gamma_sail_rad;
  // homed == "ready to be used". If false, the commands will be ignored.
  bool homed_rudder_left;
  bool homed_rudder_right;
  bool homed_sail;
};

// All values in degrees. For the drives the [-180, 180] convention is used.
// The drive interface handles the wrap around from -179 to 179 degrees.
struct DriveReferenceValuesRad {
  DriveReferenceValuesRad();
  DriveReferenceValuesRad(const DriveReferenceValues& ref_deg);
  void Reset();
  double gamma_rudder_star_left_rad;
  double gamma_rudder_star_right_rad;
  double gamma_sail_star_rad;
};

*/

#endif  // HELMSMAN_FILL_H
