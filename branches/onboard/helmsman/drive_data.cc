// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "drive_data.h"

#include <math.h>
#include "lib/check.h"
#include "lib/convert.h"

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
  int s = snprintf(line, sizeof line,
      "rudder_l_deg:%f rudder_r_deg:%f sail_deg:%f\n",
      gamma_rudder_star_left_deg, gamma_rudder_star_right_deg,
      gamma_sail_star_deg);
  return std::string(line, s);
}

void DriveReferenceValues::Reset() {
  gamma_rudder_star_left_deg = 0;
  gamma_rudder_star_right_deg = 0;
  gamma_sail_star_deg = 0;
}

void DriveReferenceValues::Check() {
  // mechanical limits, rudder might block
  CHECK_IN_INTERVAL(-25, gamma_rudder_star_left_deg, 90);
  CHECK_IN_INTERVAL(-90, gamma_rudder_star_right_deg, 25);
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

std::string DriveActualValuesRad::ToString() const {
  // used for logging only.
  char line[1024];
  int s = snprintf(line, sizeof line,
      "rudd_L_rad:%f rudd_R_rad:%f sail_rad:%f homed_L:%d homed_R:%d homed_S:%d\n",
      gamma_rudder_left_rad, gamma_rudder_right_rad, gamma_sail_rad,
      homed_rudder_left ? 1 : 0,
      homed_rudder_right ? 1 : 0,
      homed_sail ? 1 : 0);
  return std::string(line, s);
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

void DriveReferenceValuesRad::Check() {
  // mechanical limits, rudder might block
  CHECK_IN_INTERVAL(Deg2Rad( -25), gamma_rudder_star_left_rad,  Deg2Rad(90));
  CHECK_IN_INTERVAL(Deg2Rad( -90), gamma_rudder_star_right_rad, Deg2Rad(25));
  CHECK_IN_INTERVAL(Deg2Rad(-180), gamma_sail_star_rad,         Deg2Rad(180));
}

void DriveReferenceValuesRad::Reset() {
  gamma_rudder_star_left_rad  = 0;
  gamma_rudder_star_right_rad = 0;
  gamma_sail_star_rad = 0;
}

std::string DriveReferenceValuesRad::ToString() const {
  char line[1024];
  int s = snprintf(line, sizeof line,
     "rudder_l_rad:%lf rudder_r_rad:%lf sail_rad:%lf\n",
      gamma_rudder_star_left_rad, gamma_rudder_star_right_rad,
      gamma_sail_star_rad);
  return std::string(line, s);
}

void DriveReferenceValuesRad::FromProto(const RudderProto& sts) {
  gamma_rudder_star_left_rad  = Deg2Rad(sts.rudder_l_deg);
  gamma_rudder_star_right_rad = Deg2Rad(sts.rudder_r_deg);
  gamma_sail_star_rad         = Deg2Rad(sts.sail_deg);
}

void DriveReferenceValuesRad::ToProto(RudderProto* sts) const {
  sts->rudder_l_deg = Rad2Deg(gamma_rudder_star_left_rad);
  sts->rudder_r_deg = Rad2Deg(gamma_rudder_star_right_rad);
  sts->sail_deg     = Rad2Deg(gamma_sail_star_rad);
}

// This assumes a fully valid input proto.
void DriveActualValuesRad::FromProto(const RudderProto& status) {
  gamma_rudder_left_rad  = Deg2Rad(status.rudder_l_deg);
  gamma_rudder_right_rad = Deg2Rad(status.rudder_r_deg);
  gamma_sail_rad         = Deg2Rad(status.sail_deg);
  homed_rudder_left = !isnan(status.rudder_l_deg);
  homed_rudder_right = !isnan(status.rudder_r_deg);
  homed_sail = !isnan(status.sail_deg);
}

void DriveActualValuesRad::ToProto(RudderProto *status,
                                   int64_t timestamp_ms) const {
  status->timestamp_ms = timestamp_ms;
  status->rudder_l_deg =
      homed_rudder_left ? Rad2Deg(gamma_rudder_left_rad) : NAN;
  status->rudder_r_deg =
      homed_rudder_right ? Rad2Deg(gamma_rudder_right_rad) : NAN;
  status->sail_deg =
      homed_sail ? Rad2Deg(gamma_sail_rad) : NAN;
}
