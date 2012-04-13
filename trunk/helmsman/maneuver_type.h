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

ManeuverType FindManeuverType(double old_phi_z,
                              double new_phi_z,
                              double true_direction);

const char* ManeuverToString(ManeuverType maneuver_type);

#endif   // HELMSMAN_MANEUVER_TYPE_H
