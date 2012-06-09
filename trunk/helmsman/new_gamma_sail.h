// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#ifndef HELMSMAN_NEW_GAMMA_SAIL_H
#define HELMSMAN_NEW_GAMMA_SAIL_H

#include "helmsman/maneuver_type.h"
#include "helmsman/sail_controller.h"

// When the desired heading alpha_star or the wind changes
// we might need to plan a maneuver consisting of fast
// synchronous motions of boat and sail.
// When planning a maneuver we need
// to come up with the new sail angle, the direction to turn the sail
// and the maneuver type.


// This simple routine assumes that a maneuver is more or less symmetrical
// in relation to the wind.
// N.B. a tack is assymmetrical due to the intentional overshoot.
void NewGammaSail(double old_gamma_sail,
                  ManeuverType maneuver_type,
                  double overshoot,
                  double* new_gamma_sail,
                  double* delta_gamma_sail);                 // delta_gamma_sail is produced here, because
                                                             // delta is not always new-old. In some cases (jibes)
                                                             // the sail must turn by more than 180 degrees.
#endif   // HELMSMAN_NEW_GAMMA_SAIL_H
