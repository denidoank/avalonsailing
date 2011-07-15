// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#ifndef HELMSMAN_WIND_H
#define HELMSMAN_WIND_H

#include <stdio.h>
#include <string>

#include "common/check.h"
#include "common/convert.h"
#include "common/normalize.h"

// Original units from Wind sensor, relative to mast.
struct WindSensor {
  WindSensor() {
    Reset();
  }
  void Reset() {
    mag_kn = 0;  
    alpha_deg = 0;
  }
  std::string ToString() const {
  char line[1024];
  int s = snprintf(line, sizeof line, "mag_kn:%f alpha_deg:%f \n",
                   mag_kn, alpha_deg);
  return std::string(line, s);
}
  
  double mag_kn;     // in knots
  double alpha_deg;  // [0, 360]
};

// Controller needs metric units and radians.
struct WindRad {
  WindRad() {
    Reset();
  }
  WindRad(const WindSensor& wind) {
    CHECK_GE(wind.mag_kn, 0);
    mag_m_s = KnotsToMeterPerSecond(wind.mag_kn);  
    alpha_rad = SymmetricRad(Deg2Rad(wind.alpha_deg));
  }
  void Reset() {
    mag_m_s = 0;  
    alpha_rad = 0;
  }
  double alpha_rad;   // [-pi, pi]
  double mag_m_s;     // in m/s
};

#endif  // HELMSMAN_WIND_H
