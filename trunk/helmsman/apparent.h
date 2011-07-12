// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#ifndef HELMSMAN_APPARENT_H
#define  HELMSMAN_APPARENT_H

#include "helmsman/maneuver_type.h"
#include "common/polar.h"
#include "helmsman/sail_controller.h"

// Calculate the apparent wind in boat frame
// Units: All angles in radians, all magnitiudes in identical units.
// All angles have identical orientation, e.g. clockwise.

// The true wind is the true wind vector direction in the global frame,
// i.e. an angle of 0 blows North bound. 
// The boat motion vector is the motion vector of the boat in the global frame,
// e.g. if the boat goes East, then angle_boat is pi/2.
// The apparent wind is the vector of the wind relative to the boat frame, i.e.
// the boats x-axis, which points forward.
// If the wind blows to portside, the angle_app is -pi/2.
//
// The following vector equation is true (all in global frame):
// v_boat + v_app_global = v_true 

// where alpha(v_app_global) = phi_z + alpha(v_app_boat)
// One may assume, that the boat moves exactly forward and therefore the
// angle of the boats motion is equal to phi_z but this is an approximation
// only.


void Apparent(double angle_true, double mag_true,
              double angle_boat, double mag_boat,
              double phi_z,              
              double* angle_app, double* mag_app);

void ApparentPolar(const Polar& true_wind,
                   const Polar& boat_speed,
                   double phi_z,
                   Polar* apparent_wind_on_boat);

void TruePolar(const Polar& apparent_wind_on_boat,
               const Polar& boat_speed,
               double phi_z,
               Polar* true_wind);
                   
#endif   //  HELMSMAN_APPARENT_H
