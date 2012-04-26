// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, March 2012
#include "helmsman/compass.h"

#include <math.h>
#include <stdio.h>

#include "common/check.h"
#include "common/convert.h"
#include "common/normalize.h"

namespace {
// magnetic bearing is true geographic bearing plus declination.
static const double declination = Deg2Rad(1);  // +1 is East
static const double inclination = Deg2Rad(61);
}

bool CheckAccMagnitude(double acc_x,
                       double acc_y,
                       double acc_z) {
  const double acc_mag_min = 8 * 8;  // m/s^2
  const double acc_mag_max = 12 * 12;
  double acc_magnitude_squared = acc_x * acc_x +
                                 acc_y * acc_y +
                                 acc_z * acc_z;
  return acc_magnitude_squared >= acc_mag_min && acc_magnitude_squared <= acc_mag_max;
}


bool GravityVectorToPitchAndRoll(double acc_x,
                                 double acc_y,
                                 double acc_z,
                                 double* pitch_rad,
                                 double* roll_rad) {
  if (!CheckAccMagnitude(acc_x, acc_y, acc_z))
    return false;
  *pitch_rad = atan2(+acc_x, -acc_z);
  *roll_rad  = atan2(-acc_y, -acc_z);
  //fprintf(stderr, "acc pitch: %6.2lf deg, acc roll %6.2lf deg\n", Rad2Deg(*pitch_rad), Rad2Deg(*roll_rad));

  const double limit_squared = Deg2Rad(30) * Deg2Rad(30);
  return *pitch_rad * *pitch_rad + *roll_rad * *roll_rad < limit_squared;
}

// produce geographic bearing
bool VectorsToBearing(double acc_x,
                      double acc_y,
                      double acc_z,
                      double mag_x,
                      double mag_y,
                      double mag_z,
                      double* bearing_rad) {
  double pitch_rad;
  double roll_rad;
  bool valid = GravityVectorToPitchAndRoll(acc_x,
                                           acc_y,
                                           acc_z,
                                           &pitch_rad,
                                           &roll_rad);
  double proj_x = 0;
  double proj_y = 0;
  if (valid) {
    proj_x = mag_x * cos(pitch_rad) +
             mag_y * sin(roll_rad) * sin(pitch_rad) -
             mag_z * cos(roll_rad) * sin(pitch_rad);
    proj_y = mag_y * cos(roll_rad) +
             mag_z * sin(roll_rad);
    // We like to get the boats bearing relative to the magnetic vector
    // so we invert the angle.
    CHECK(declination>0);
    *bearing_rad = SymmetricRad(-atan2(proj_y, proj_x));
  }
  return valid;
}

// Assume that pitch and roll are zero.
// Input is the geographic bearing (not magnetic).
void BearingToMagnetic(double magnetic_phi_z, double* mag_x,  double* mag_y, double* mag_z) {
  const double mag_au = 47;  // microTesla
  *mag_x = mag_au * cos(-magnetic_phi_z) * cos(inclination);
  *mag_y = mag_au * sin(-magnetic_phi_z) * cos(inclination);
  *mag_z = mag_au * sin(inclination);
}

double GeographicToMagnetic(double geographic) {
  return geographic + declination;
}

double MagneticToGeographic(double magnetic) {
  return magnetic - declination;
}
