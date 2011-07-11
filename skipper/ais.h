// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011
#ifndef SKIPPER_AIS_H
#define SKIPPER_AIS_H

#include <string.h>

// position and speed of a ship
struct AISInfo {
  double lat_deg;
  double lon_deg;
  double speed_kn;
  std::string id;
};

#endif  // SKIPPER_AIS_H
