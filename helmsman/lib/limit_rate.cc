// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Rate limit radians angles with correct handling of wrap arounds.

#include <math.h>
#include "common/check.h"
#include "common/normalize.h"

namespace {

double Saturate(double x, double limit) { 
  CHECK_GT(limit, 0);
  if (x >  limit)
    return  limit;
  if (x < -limit)
    return -limit;
  return x;
}

}

void LimitRate(double in, double max_delta, double* follows) {
  double delta = in - *follows;
  *follows += Saturate(delta, max_delta);
}

void LimitRateWrapRad(double in, double max_delta, double* follows) {
  double delta = in - *follows;
  // underflow
  if (delta > M_PI) {
    delta -= 2 * M_PI;
  }
  // overflow
  if (delta < -M_PI) {
    delta += 2 * M_PI;
  }
    
  *follows += Saturate(delta, max_delta);
  *follows = SymmetricRad(*follows);
}

