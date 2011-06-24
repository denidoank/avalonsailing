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
void Apparent(double angle_true, double mag_true,
              double angle_boat, double mag_boat,
              double* angle_app, double* mag_app);

void ApparentPolar(const Polar& true_wind,
                   const Polar& boat_speed,
                   Polar* apparent_wind_on_boat);
#endif   //  HELMSMAN_APPARENT_H
