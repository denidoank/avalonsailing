// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#ifndef HELMSMAN_WIND_SENSOR_H
#define HELMSMAN_WIND_SENSOR_H

#include <stdio.h>
#include <string>

#include "common/check.h"
#include "common/convert.h"
#include "common/normalize.h"
#include "proto/wind.h"


// Original units from Wind sensor, relative to mast.
struct WindSensor {
  WindSensor();
  void Reset();
  std::string ToString() const;
  void ToProto(WindProto* wind_sensor_proto) const;
  
  double mag_m_s;    // in m/s
  double alpha_deg;  // [0, 360), where the wind is coming FROM
  bool valid;        // sensor selfcheck output
};

#endif  // HELMSMAN_WIND_SENSOR_H
