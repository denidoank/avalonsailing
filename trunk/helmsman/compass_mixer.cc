// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, March 2012
#include "helmsman/compass_mixer.h"

#include <math.h>
#include <stdio.h>

#include "common/check.h"
#include "common/convert.h"
#include "common/normalize.h"

extern int debug;

double CompassMixer::Mix(double alpha1_rad, double weight1,
                         double alpha2_rad, double weight2,
                         double alpha3_rad, double weight3,
                         bool* valid) {
  input_[0] = alpha1_rad;
  weight_[0] = weight1;
  input_[1] = alpha2_rad;
  weight_[1] = weight2;
  input_[2] = alpha3_rad;
  weight_[2] = weight3;
  for (int n = 0; n < 3; ++n) {
    CHECK(!isnan(weight_[n]));
    CHECK_IN_INTERVAL(0, weight_[n], 1);
    if (isnan(input_[n])) {
      input_[n] = 0;
      weight_[n] = 0;
    }
    CHECK_IN_0_2PI(input_[n]);
  }

  double x = 0;
  double y = 0;
  double sum_of_weights = 0;
  for (int n = 0; n < 3; ++n) {
    sum_of_weights += weight_[n];
    x += weight_[n] * cos(input_[n]);
    y += weight_[n] * sin(input_[n]);
  }

  double result = atan2(y, x);
  // 1 means all inputs are parallel, 0.7 90 degree , 0.5 120 degree divergence of equally weighted inputs.
  // divergence can be caused by quick changes and different filtering delays.
  double consensus = sqrt(x * x + y * y) / sum_of_weights;
  if (sum_of_weights < 0.5 || consensus < 0.5) {
    fprintf(stderr, "Compassmixer no censensus(%6.4lf): KFL %lg (%6.4lf), IMU %6.4lf (%6.4lf), CMP %6.4lf (%6.4lf)",
            consensus, alpha1_rad, weight1, alpha2_rad, weight2, alpha3_rad, weight3);
    *valid = false;
    return 0;
  } else {
    if (debug)
      fprintf(stderr, "Compassmixer censensus is %6.4lf: KFL %6.4lf (%6.4lf), IMU %6.4lf (%6.4lf), CMP %6.4lf (%6.4lf) -> %6.4lf",
            consensus, alpha1_rad, weight1, alpha2_rad, weight2, alpha3_rad, weight3, NormalizeRad(result));
    *valid = true;
    return NormalizeRad(result);
  }
}
