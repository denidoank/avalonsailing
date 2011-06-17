// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef COMMON_DELTA_ANGLE_H
#define COMMON_DELTA_ANGLE_H

// Functions to handle [-pi, pi) angles and their differences a2 - a1.

// A delta function for symmetric angles in [-180, 180)
// such that SymmetricNormalize(a1 + Delta(a1, a2)) = a2
// and fabs(Delta(a1, a2)) <= 180
double DeltaRad(double a1_rad, double a2_rad);
double DeltaDeg(double a1_deg, double a2_deg);

// Out of the 2 options return the one with less way to go to the target.
// Returns option1 at equal distance
double NearerRad(double target_rad, double option1_rad, double option2_rad);
// A variant for degrees.
double NearerDeg(double target_deg, double option1_deg, double option2_deg);

#endif  // COMMON_DELTA_ANGLE_H
