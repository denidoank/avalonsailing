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
#include "proto/wind.h"


// Original units from Wind sensor, relative to mast.
struct WindSensor {
  WindSensor() {
    Reset();
  }
  void Reset() {
    mag_m_s = 0;  
    alpha_deg = 0;
  }
  std::string ToString() const {
  char line[1024];
  int s = snprintf(line, sizeof line, "mag_m_s:%f alpha_deg:%f \n",
                   mag_m_s, alpha_deg);
  return std::string(line, s);
  }
  void ToProto(WindProto* wind_sensor_proto) const {
    wind_sensor_proto->angle_deg = alpha_deg;
    wind_sensor_proto->speed_m_s = mag_m_s;
  }
  
  double mag_m_s;     // in m/s
  double alpha_deg;  // [0, 360], where the wind is coming FROM
};

// Controller needs metric units and radians.
struct WindRad {
  WindRad() {
    Reset();
  }
  WindRad(const WindSensor& wind) {
    CHECK_GE(wind.mag_m_s, 0);
    mag_m_s = KnotsToMeterPerSecond(wind.mag_m_s);  
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
