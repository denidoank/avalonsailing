// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/maneuver_type.h"

#include "common/delta_angle.h"
#include "common/sign.h"


// old and new angle of heading relative to true wind
ManeuverType FindManeuverType(double old_phi_z,
                              double new_phi_z,
                              double true_direction) {
  double old_delta = DeltaOldNewRad(true_direction, old_phi_z);                                  
  double new_delta = DeltaOldNewRad(true_direction, new_phi_z);                                  
  double turn = DeltaOldNewRad(old_phi_z, new_phi_z);   

  // If the motion direction crosses the true wind vector, then it is a jibe.
  if (old_delta * new_delta < 0 &&
      turn * old_delta < 0)
    return kJibe;
  // The opposite case: a tack.     
  if (Reverse(old_delta) * Reverse(new_delta) < 0 &&
      turn * Reverse(old_delta) < 0)
    return kTack;
  return kChange;  
}
