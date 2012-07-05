// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

#include <math.h>

// Force angle into [0, 360).
double NormalizeDeg(double alpha_deg) {
  double x = fmod(alpha_deg, 360.0);
  if (x >= 0)
    return x;
  else
    return 360.0 + x;
}

// Force result into [-180, 180).
double SymmetricDeg(double alpha_deg) {
  return NormalizeDeg(alpha_deg + 180.0) - 180.0;
}

// Force radians into [0, 2*pi).
double NormalizeRad(double alpha_rad) {
  double x = fmod(alpha_rad, 2 * M_PI);
  if (x >= 0)
    return x;
  else
    return 2 * M_PI + x;
}

// Force result into [-pi, pi).
double SymmetricRad(double alpha_rad) {
  return NormalizeRad(alpha_rad + M_PI) - M_PI;
}
