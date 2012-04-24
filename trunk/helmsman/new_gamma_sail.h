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

// This old and complex routine is exact but requires the true wind
// and boat motion data. I kept this as a reference in tests.
// Use NextGammaSail for the boat!
void NewGammaSail(double alpha_true, double mag_true,
                  double alpha_boat, double mag_boat,
                  double new_alpha_boat,
                  SailController* sail_controller,
                  double* new_gamma_sail,
                  double* delta_gamma_sail,
                  ManeuverType* maneuver_type);

void NewGammaSailWithOldGammaSail(
                  double alpha_true, double mag_true,
                  double alpha_boat, double mag_boat,
                  double new_alpha_boat,
                  double old_gamma_sail,
                  ManeuverType maneuver_type,
                  SailController* sail_controller,
                  double* new_gamma_sail,
                  double* delta_gamma_sail);


// We don't want to depend on the boat speed because the IMU does
// not deliver this information reliably. We assume the following:
// The boat speed is a fraction of the wind speed around 0.25.
// (We might read this ratio from the experimentally verified Polardiagram.)
// Then we can do an estimation with less than 10 degree error, which
// is good enough and robust against boat speed errors.
// In the test we compare this approximation with the precise result.
void NextGammaSailWithOldGammaSail(
                  double old_alpha_app, double old_mag_app,  // apparent wind, angle and magnitude
                  double old_alpha_boat,                     // boat direction old
                  double new_alpha_boat,                     // boat direction new
                  double old_gamma_sail,
                  ManeuverType maneuver_type,
                  SailController* sail_controller,
                  double* new_gamma_sail,
                  double* delta_gamma_sail);                 // delta_gamma_sail is produced here, because
                                                             // delta is not always new-old. In some cases (jibes)
                                                             // the sail must turn by more than 180 degrees.

#endif   // HELMSMAN_NEW_GAMMA_SAIL_H
