// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/drive_data.h" 

#include "common/check.h" 
#include "common/convert.h" 

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
  CHECK_INTERVAL(-180, gamma_rudder_left_deg, 180);
  CHECK_INTERVAL(-180, gamma_rudder_right_deg, 180);
  CHECK_INTERVAL(-180, gamma_sail_deg, 180);
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
  
void DriveReferenceValues::Reset() {
  gamma_rudder_star_left_deg = 0;
  gamma_rudder_star_right_deg = 0;
  gamma_sail_star_deg = 0;
}

void DriveReferenceValues::Check() {
  CHECK_INTERVAL(-180, gamma_rudder_star_left_deg, 180);
  CHECK_INTERVAL(-180, gamma_rudder_star_right_deg, 180);
  CHECK_INTERVAL(-180, gamma_sail_star_deg, 180);
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
