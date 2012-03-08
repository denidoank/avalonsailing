// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, March 2012
#ifndef HELMSMAN_COMPASS_H
#define HELMSMAN_COMPASS_H

// Code to get the boats bearing phi_z from electronic compass signals.
// They are delivered by the IMU.
// We assume that the compass is mounted with its x-axis pointing forward in the boat.

// Returns true if pitch and roll are within a usable range (e.g. +-30 degrees)
bool GravityVectorToPitchAndRoll(double acc_x,
                                 double acc_y,
                                 double acc_z,
                                 double* pitch_rad,
                                 double* roll_rad);

// Returns true if the output is reliable.
bool VectorsToBearing(double acc_x,
                      double acc_y,
                      double acc_z,
                      double mag_x,
                      double mag_y,
                      double mag_z,
                      double* bearing_rad);

#endif   // HELMSMAN_COMPASS_H
