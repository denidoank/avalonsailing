// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#ifndef HELMSMAN_MANEUVER_TYPE_H
#define HELMSMAN_MANEUVER_TYPE_H

enum ManeuverType {
  kChange,  // of direction only
  kTack,
  kJibe
};

// This works for the apparent wind angles (relative to the boat axis) as well
// as for (alpha_true - alpha_star), i.e. the exact vector calculation to
// calculate the apparent wind is not necessary here. 
ManeuverType FindManeuverType(double old_apparent_angle,
                              double new_apparent_angle);

#endif   // HELMSMAN_MANEUVER_TYPE_H
