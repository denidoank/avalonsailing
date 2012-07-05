// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#ifndef HELMSMAN_OPTIMAL_GAMMA_SAIL_H
#define HELMSMAN_OPTIMAL_GAMMA_SAIL_H

// Returns optimal gamma sail for the given
// direction of the apparent wind vector relative to the boats x-axis
// (in degrees, 90 degrees is wind from port side, 0 degree is wind
// from behind) in [0, 360) and
// the wind speed in m/s. wind_speed >= 0 
double OptimalGammaSail(double alpha_apparent_wind, double wind_speed);

// As OptimalGammaSail. Additionally the forward force F_x in Newton and
// the heel angle, rotation around the x-axis, indegrees is produced. 
void OptimalTriplett(double alpha_apparent_wind,
                     double magnitude_wind,
                     double* gamma_sail,
                     double* force,
                     double* heel);
#endif  // HELMSMAN_OPTIMAL_GAMMA_SAIL_H
