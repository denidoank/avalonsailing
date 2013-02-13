// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/new_gamma_sail.h"

#include "common/polar.h"
#include "common/sign.h"

#include <math.h>
#include <stdio.h>

extern int debug;

// All maneuvers are minimal and symmetric to the wind axis,
// i.e. a tack turns from close reach to close reach
// and a jibe turns just through the jibe zone. This is guaranteed by
// NormalController::LimitToMinimalManeuver.
// This means
// - Tacks and jibes are more or less symmetrical in reference to the wind axis.
// - The apparent wind changes in a symmetrical fashion as well.
// - The resulting gamma_sail is also symmetrical.
// However, for tacks we get an asymmetry due to the intentional overshoot.

double NewGammaSail(Angle old_gamma_sail,
                    ManeuverType maneuver_type,
                    Angle overshoot) {
  Angle new_gamma_sail;
  switch(maneuver_type) {
    case kJibe:
      new_gamma_sail = -old_gamma_sail;
      break;
    case kTack:
      new_gamma_sail = -old_gamma_sail - overshoot * old_gamma_sail.sign();
      break;
    case kChange:
      // When used with the NormalController this branch is never used.
      CHECK(0);
      break;
    default:
      CHECK(0);
      break;
  }
  if (debug) {
    fprintf(stderr, "new_gamma_sail %6.2lf deg\n", new_gamma_sail.deg());
  }

  double delta = new_gamma_sail.rad() - old_gamma_sail.rad();
  if (maneuver_type == kJibe)
    return delta - 2 * M_PI * Sign(delta);
  else
    return delta;
}
