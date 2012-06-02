// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/new_gamma_sail.h"

#include "common/delta_angle.h"
#include "common/normalize.h"
#include "common/polar.h"
#include "common/sign.h"

#include <math.h>
#include <stdio.h>

// All maneuvers are minimal and symmetric to the wind axis,
// i.e. a tack turns from close reach to close reach
// and a jibe turns just through the jibe zone. This is guaranteed by
// NormalController::LimitToMinimalManeuver.
// This means
// - Tacks and jibes are more or less symmetrical in reference to the wind axis.
// - The apparent wind changes in a symmetrical fashion as well.
// - The resulting gamma_sail is also symmetrical.
// However, for tacks we get an asymmetry diu to the intentional overshoot.

void NewGammaSail(double old_gamma_sail,  // star
                  ManeuverType maneuver_type,
                  double alpha_true_wind,
                  double overshoot,
                  double* new_gamma_sail,
                  double* delta_gamma_sail) {
  old_gamma_sail = SymmetricRad(old_gamma_sail);
  double sail_to_wind = DeltaOldNewRad(alpha_true_wind, old_gamma_sail);

  fprintf(stderr, "old_gamma_sail %6.2lf deg\n", Rad2Deg(old_gamma_sail));
  fprintf(stderr, "alpha_true_wind %6.2lf deg\n", Rad2Deg(alpha_true_wind));
  fprintf(stderr, "sail_to_wind %6.2lf deg\n", Rad2Deg(sail_to_wind));

  switch(maneuver_type) {
    case kJibe:
      *new_gamma_sail = SymmetricRad(alpha_true_wind - sail_to_wind);
      fprintf(stderr, "*new_gamma_sail %6.2lf deg\n", Rad2Deg(*new_gamma_sail));
      break;
    case kTack:
      *new_gamma_sail = SymmetricRad(alpha_true_wind - sail_to_wind - Sign(sail_to_wind) * overshoot);
      fprintf(stderr, "*new_gamma_sail %6.2lf deg\n", Rad2Deg(*new_gamma_sail));
      break;
    case kChange:
      // When used with the NormalController this branch is never used.
      CHECK(0);
      break;
    default:
      CHECK(0);
      break;
  }
  double delta = *new_gamma_sail - old_gamma_sail;
  // fprintf(stderr, "old gamma: %6.2f deg\n", Rad2Deg(old_gamma_sail));
  // fprintf(stderr, "delta: %6.2f deg %g\n", Rad2Deg(delta), Sign(delta));

  if (maneuver_type == kJibe)
    *delta_gamma_sail = delta - 2 * M_PI * Sign(delta);
  else
    *delta_gamma_sail = delta;
}
