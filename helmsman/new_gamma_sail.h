// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#ifndef HELMSMAN_NEW_GAMMA_SAIL_H
#define HELMSMAN_NEW_GAMMA_SAIL_H

#include "helmsman/maneuver_type.h"
#include "helmsman/sail_controller.h"

// When the desired heading alpha_star or the wind changes we need
// to come up with the new sail angle, the direction to turn the sail
// and the maneuver type. 

// or make interface with Polar
void NewGammaSail(double alpha_true, double mag_true,
                  double alpha_boat, double mag_boat,
                  double new_alpha_boat,
                  SailController* sail_controller,
                  double* new_gamma_sail,
                  double* delta_gamma_sail,
                  ManeuverType* maneuver_type);

#endif   // HELMSMAN_NEW_GAMMA_SAIL_H

