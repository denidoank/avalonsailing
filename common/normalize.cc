// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

#include <math.h>

// Force angle into [0, 360).
double NormalizeAngle(double alpha) {
  double x = fmod(alpha, 360.0);
  if (x >= 0)
    return x;
  else
    return 360.0 + x;
}

// Force result into (-180, 180].
double SymmetricAngle(double alpha) {
  return drem(alpha, 360.0);
}

// Force radians into [0, 2*pi).
double NormalizeRadians(double alpha) {
  double x = fmod(alpha, 2 * M_PI);
  if (x >= 0)
    return x;
  else
    return 2 * M_PI + x;
}

// Force result into (-pi, pi].
double SymmetricRadians(double alpha) {
  return drem(alpha, 2 * M_PI);
}

