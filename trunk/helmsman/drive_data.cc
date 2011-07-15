// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "drive_data.h"

#include "common/check.h"
#include "common/convert.h"

#include <stdio.h>
#include <string.h>

DriveActualValues::DriveActualValues() {
  Reset();
}

void DriveActualValues::Reset() {
  gamma_rudder_left_deg = 0;
  gamma_rudder_right_deg = 0;
  gamma_sail_deg = 0;
  homed_rudder_left = false;
  homed_rudder_right = false;
  homed_sail = false;
}

void DriveActualValues::Check() {
  CHECK_IN_INTERVAL(-180, gamma_rudder_left_deg, 180);
  CHECK_IN_INTERVAL(-180, gamma_rudder_right_deg, 180);
  CHECK_IN_INTERVAL(-180, gamma_sail_deg, 180);
}


DriveReferenceValues::DriveReferenceValues() {
  Reset();
}

DriveReferenceValues::DriveReferenceValues(
    const DriveReferenceValuesRad& ref_rad) {
  gamma_rudder_star_left_deg  = Rad2Deg(ref_rad.gamma_rudder_star_left_rad);
  gamma_rudder_star_right_deg = Rad2Deg(ref_rad.gamma_rudder_star_right_rad);
  gamma_sail_star_deg = Rad2Deg(ref_rad.gamma_sail_star_rad);
}

std::string DriveReferenceValues::ToString() const {
  char line[1024];
  int s = snprintf(line, sizeof line, "rudder_l_deg:%f rudder_r_deg:%f sail_deg:%f\n",
      gamma_rudder_star_left_deg, gamma_rudder_star_right_deg, gamma_sail_star_deg);
  return std::string(line, s);
}

void DriveReferenceValues::Reset() {
  gamma_rudder_star_left_deg = 0;
  gamma_rudder_star_right_deg = 0;
  gamma_sail_star_deg = 0;
}

void DriveReferenceValues::Check() {
  CHECK_IN_INTERVAL(-180, gamma_rudder_star_left_deg, 180);
  CHECK_IN_INTERVAL(-180, gamma_rudder_star_right_deg, 180);
  CHECK_IN_INTERVAL(-180, gamma_sail_star_deg, 180);
}


// Internally used radians versions.
DriveActualValuesRad::DriveActualValuesRad() {
  Reset();
}

DriveActualValuesRad::DriveActualValuesRad(const DriveActualValues& act_deg) {
  gamma_rudder_left_rad = Deg2Rad(act_deg.gamma_rudder_left_deg);
  gamma_rudder_right_rad = Deg2Rad(act_deg.gamma_rudder_right_deg);
  gamma_sail_rad = Deg2Rad(act_deg.gamma_sail_deg);
  homed_rudder_left = act_deg.homed_rudder_left;
  homed_rudder_right = act_deg.homed_rudder_right;
  homed_sail = act_deg.homed_sail;
}

void DriveActualValuesRad::Reset() {
  gamma_rudder_left_rad = 0;
  gamma_rudder_right_rad = 0;
  gamma_sail_rad = 0;
  homed_rudder_left = false;
  homed_rudder_right = false;
  homed_sail = false;
}


DriveReferenceValuesRad::DriveReferenceValuesRad() {
  Reset();
}

DriveReferenceValuesRad::DriveReferenceValuesRad(
    const DriveReferenceValues& ref_deg) {
  gamma_rudder_star_left_rad  = Deg2Rad(ref_deg.gamma_rudder_star_left_deg);
  gamma_rudder_star_right_rad = Deg2Rad(ref_deg.gamma_rudder_star_right_deg);
  gamma_sail_star_rad = Deg2Rad(ref_deg.gamma_sail_star_deg);
}

void DriveReferenceValuesRad::Reset() {
  gamma_rudder_star_left_rad  = 0;
  gamma_rudder_star_right_rad = 0;
  gamma_sail_star_rad = 0;
}

bool DriveReferenceValuesRad::operator!=(const DriveReferenceValuesRad& r) {
  return gamma_rudder_star_left_rad !=  r.gamma_rudder_star_left_rad ||
         gamma_rudder_star_right_rad != r.gamma_rudder_star_right_rad ||
         gamma_sail_star_rad !=         r.gamma_sail_star_rad;
}  

DriveActualValuesRad::DriveActualValuesRad(const std::string& kvline) {
  Reset();
  const char* line = kvline.c_str();
  while (*line) {
    char key[16];
    double value;
    int skip = 0;
    int n = sscanf(line, "%16[a-z_]:%lf %n", key, &value, &skip);
    CHECK(n >= 2);    // invalid line
    CHECK(skip > 0);  // invalid line
    line += skip;
    CHECK(line - kvline.c_str() <= kvline.size());  // really a bug

    if (!strcmp(key, "rudder_l_deg")) { gamma_rudder_left_rad  = Deg2Rad(value); homed_rudder_left = true; continue; }
    if (!strcmp(key, "rudder_r_deg")) { gamma_rudder_right_rad = Deg2Rad(value); homed_rudder_right = true;continue; }
    if (!strcmp(key, "sail_deg"    )) { gamma_sail_rad         = Deg2Rad(value); homed_sail = true;        continue; }
    CHECK(false);  // if we get here there is an unrecognized key.
  }
}