// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011
#include "skipper/target_circle_cascade.h"

class Planner {
 public:
  static double /* alpha_star_deg to our target */ ToDeg(double latitude_deg,   // North, degrees
                                                         double longitude_deg); // East, degrees
  static void Init(const LatLon& lat_lon);
  static bool TargetReached(const LatLon& lat_lon);
  static bool Initialized();
  static void Reset();
  static void SimplePlan(double lat_deg, double lon_deg);
 private:
  static bool initialized_;
  static TargetCircleCascade plan_;
  static double alpha_star_;
  static double last_turn_time_;
};




