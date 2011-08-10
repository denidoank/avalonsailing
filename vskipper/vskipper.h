// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef VSKIPPER_VSKIPPER_H
#define VSKIPPER_VSKIPPER_H

#include <stdint.h>
#include <string>
#include <vector>

#include "util.h"

namespace skipper {

struct AvalonState {
  uint64_t timestamp_ms;
  LatLon position;
  // Desired direction (according to global planner)
  Bearing target;
  // Current wind (used to estimate expected speed in every direction)
  Bearing wind;  // Where the wind is coming FROM
  double wind_speed_m_s;

  AvalonState() : timestamp_ms(0), target(Bearing::West()), wind_speed_m_s(0) {}
};

struct AisInfo {
  uint64_t timestamp_ms;
  LatLon position;
  Bearing bearing;
  double speed_m_s;
  std::string id;  // For debug logging only

  AisInfo() : timestamp_ms(0), speed_m_s(0) {}
};

Bearing RunVSkipper(const AvalonState& now,
                    const std::vector<AisInfo>& ais,
                    int debug);
}  // skipper

#endif  // VSKIPPER_VSKIPPER_H
