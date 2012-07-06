// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// Functions to handle [-pi, pi) angles and their differences
#include "delta_angle.h"

#include <math.h>
#include "normalize.h"

double DeltaOldNewRad(double old_rad, double new_rad) {
  return SymmetricRad(new_rad - old_rad);
}

double DeltaOldNewDeg(double old_deg, double new_deg) {
  return SymmetricDeg(new_deg - old_deg);
}

// Out of the 2 options return the one with less way to go to the target.
// option1 at equal distance
double NearerRad(double target_rad, double option1_rad, double option2_rad) {
  double d1 = DeltaOldNewRad(target_rad, option1_rad);
  double d2 = DeltaOldNewRad(target_rad, option2_rad);
  if (fabs(d1) < fabs(d2))
    return option1_rad;
  else
    return option2_rad;
}

// Out of the 2 options return the one with less way to go to the target.
// option1 at equal distance
double NearerDeg(double target_deg, double option1_deg, double option2_deg) {
  double d1 = DeltaOldNewDeg(target_deg, option1_deg);
  double d2 = DeltaOldNewDeg(target_deg, option2_deg);
  if (fabs(d2) < fabs(d1))
    return option2_deg;
  else
    return option1_deg;
}

double Reverse(double angle_rad) {
  return SymmetricRad(angle_rad - M_PI);
}

