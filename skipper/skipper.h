// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef SKIPPER_SKIPPER_H
#define SKIPPER_SKIPPER_H

#include <vector>

#include "helmsman/skipper_input.h"  
#include "skipper/ais.h"
#include "skipper/lat_lon.h"

class Skipper {
 public:
  // Run this occasionally, when new skipper input or AIS information is available.
  static void Run(const SkipperInput& in,
                  const std::vector<AISInfo>& ais,
                  double* alpha_star_deg);
  static void Init(const SkipperInput& in);
  static bool TargetReached(const LatLon& lat_lon);

};

#endif  // SKIPPER_SKIPPER_H
