// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef COMMON_DELTA_ANGLE_H
#define COMMON_DELTA_ANGLE_H

// Functions to handle [-pi, pi) angles and their differences a2 - a1.

bool Less(double left_rad, double right_rad);
bool LessOrEqual(double left_rad, double right_rad);

// Minimum and Maximum do not Normalize.
double Minimum(double left_rad, double right_rad);
double Maximum(double left_rad, double right_rad);

// A delta function for symmetric angles in [-pi, pi) resp. [-180, 180)
// such that SymmetricNormalize(a1 + Delta(a1, a2)) = a2
// and fabs(Delta(a1, a2)) <= 180
double DeltaOldNewRad(double old_rad, double new_rad);
double DeltaOldNewDeg(double old_deg, double new_deg);

// Out of the 2 options return the one with less way to go to the target.
// Returns option1 at equal distance
double NearerRad(double target_rad, double option1_rad, double option2_rad);
// Additionally set option1 if option1_rad was taken.
double NearerRad(double target_rad,
                 double option1_rad, double option2_rad,
                 bool* option1);

// A variant for degrees.
double NearerDeg(double target_deg, double option1_deg, double option2_deg);

double Reverse(double angle_rad);
#endif  // COMMON_DELTA_ANGLE_H
