// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, April 2012

#ifndef HELMSMAN_COMPASS_SENSOR_H
#define HELMSMAN_COMPASS_SENSOR_H

#include <stdio.h>
#include <string>

#include "proto/compass.h"

struct CompassSensor {
  CompassSensor();
  void Reset();
  std::string ToString() const;
  void ToProto(CompassProto* compass_proto) const;
  
  double phi_z_rad;  // [0, 2PI) radians, bearing of the boat
};

#endif  // HELMSMAN_COMPASS_SENSOR_H
