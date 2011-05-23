// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/optimal_gamma_sail.h"

#include <algorithm>
#include <math.h>

#include <stdio.h>

#include "common/check.h"
#include "helmsman/gamma_sail_tab_common.h"
#include "helmsman/gamma_sail_tab_3.h"
#include "helmsman/gamma_sail_tab_6.h"
#include "helmsman/gamma_sail_tab_9.h"
#include "helmsman/gamma_sail_tab_12.h"

using namespace std;

const int last_alpha_index =
    static_cast<int>((alpha_wind_max - alpha_wind_min) /
                     alpha_wind_step + 0.5);

const double magnitude_wind_step = 3;
const double magnitude_wind_min = 3;
const double magnitude_wind_max = 12;
const int last_magnitude_index =
    static_cast<int>((magnitude_wind_max - magnitude_wind_min) /
                     magnitude_wind_step + 0.5);

const double* gamma_s[4] = {gamma_sail_tab_3, gamma_sail_tab_6,
                            gamma_sail_tab_9, gamma_sail_tab_12};
const double* force_x[4] = {force_x_tab_3, force_x_tab_6,
                            force_x_tab_9, force_x_tab_12};
const double* heeling[4] = {heeling_tab_3, heeling_tab_6,
                            heeling_tab_9, heeling_tab_12};


double OptimalGammaSail(double alpha_apparent_wind, double magnitude_wind) {
  int sign;
  if (alpha_apparent_wind < 0) {
    sign = -1;
    alpha_apparent_wind = -alpha_apparent_wind;
  }

  alpha_apparent_wind = min(max(alpha_apparent_wind, alpha_wind_min), alpha_wind_max);
  double quotient_alpha = (alpha_apparent_wind - alpha_wind_min) / alpha_wind_step;
  int index_alpha_low = floor(quotient_alpha);
  int index_alpha_high = ceil(quotient_alpha);
  double weight_alpha_high = quotient_alpha - index_alpha_low;
  double weight_alpha_low = 1 - weight_alpha_high;
  
  CHECK(magnitude_wind >= 0);
  magnitude_wind = min(max(magnitude_wind, magnitude_wind_min), magnitude_wind_max);
  double quotient_mag = (magnitude_wind - magnitude_wind_min) / magnitude_wind_step;
  int index_mag_low = floor(quotient_mag);
  int index_mag_high = ceil(quotient_mag);
  double weight_mag_high = quotient_mag - index_mag_low;
  double weight_mag_low = 1 - weight_mag_high;

  double gamma_sail_opt = weight_mag_low  * (weight_alpha_low * gamma_s[index_mag_low][index_alpha_low] +
                                             weight_alpha_high * gamma_s[index_mag_low][index_alpha_high]) +
                          weight_mag_high * (weight_alpha_low * gamma_s[index_mag_high][index_alpha_low] +
                                             weight_alpha_high * gamma_s[index_mag_high][index_alpha_high]);
  return sign * gamma_sail_opt;
}

void OptimalTriplett(double alpha_apparent_wind, double magnitude_wind, double* gamma_sail, double* force, double* heel) {
  int sign;
  if (alpha_apparent_wind < 0) {
    sign = -1;
    alpha_apparent_wind = -alpha_apparent_wind;
  }

  alpha_apparent_wind = min(max(alpha_apparent_wind, alpha_wind_min), alpha_wind_max);
  double quotient_alpha = (alpha_apparent_wind - alpha_wind_min) / alpha_wind_step;
  int index_alpha_low = floor(quotient_alpha);
  int index_alpha_high = ceil(quotient_alpha);
  double weight_alpha_high = quotient_alpha - index_alpha_low;
  double weight_alpha_low = 1 - weight_alpha_high;

  printf("index_alpha_low, index_alpha_high, weight_alpha_low, weight_alpha_high %15d %15d %15g %15g\n", index_alpha_low, index_alpha_high, weight_alpha_low, weight_alpha_high);
  
  CHECK(magnitude_wind >= 0);
  magnitude_wind = min(max(magnitude_wind, magnitude_wind_min), magnitude_wind_max);
  double quotient_mag = (magnitude_wind - magnitude_wind_min) / magnitude_wind_step;
  int index_mag_low = floor(quotient_mag);
  int index_mag_high = ceil(quotient_mag);
  double weight_mag_high = quotient_mag - index_mag_low;
  double weight_mag_low = 1 - weight_mag_high;


  *gamma_sail = sign *
      (weight_mag_low  * (weight_alpha_low  * gamma_s[index_mag_low][index_alpha_low] +
                          weight_alpha_high * gamma_s[index_mag_low][index_alpha_high]) +
       weight_mag_high * (weight_alpha_low  * gamma_s[index_mag_high][index_alpha_low] +
                          weight_alpha_high * gamma_s[index_mag_high][index_alpha_high]));
  
  *force =
      weight_mag_low  * (weight_alpha_low * force_x[index_mag_low][index_alpha_low]  +
                         weight_alpha_high * force_x[index_mag_low][index_alpha_high]) +
      weight_mag_high * (weight_alpha_low  * force_x[index_mag_high][index_alpha_low] +
                         weight_alpha_high * force_x[index_mag_high][index_alpha_high]);
  
  *heel = sign *
      (weight_mag_low  * (weight_alpha_low  * heeling[index_mag_low][index_alpha_low] +
                          weight_alpha_high * heeling[index_mag_low][index_alpha_high]) +
       weight_mag_high * (weight_alpha_low  * heeling[index_mag_high][index_alpha_low] +
                          weight_alpha_high * heeling[index_mag_high][index_alpha_high]));
}

