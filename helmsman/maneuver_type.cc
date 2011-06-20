// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/maneuver_type.h"

#include "common/delta_angle.h"
#include "common/sign.h"

namespace {
double BinarySign(double x) {
  return x >= 0 ? 1 : -1;
}
}  // namespace

ManeuverType FindManeuverType(double old_apparent_angle,
                              double new_apparent_angle) { 
  if (BinarySign(old_apparent_angle) == BinarySign(new_apparent_angle))
    return kChange;
  if (BinarySign(old_apparent_angle) *
      BinarySign(DeltaRad(old_apparent_angle, new_apparent_angle)) > 0)
    return kTack;
  else
    return kJibe;
}
