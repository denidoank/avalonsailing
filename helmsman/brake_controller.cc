// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#include "common/unknown.h"  // kUnknown
#include "common/convert.h"

#include "helmsman/brake_controller.h"  

extern int debug;

const double kRudderBrakeAngleRad = Deg2Rad(80);

BrakeController::~BrakeController() {}

void BrakeController::Run(const ControllerInput& in,
                          const FilteredMeasurements& filtered,
                          ControllerOutput* out) {
  out->Reset();
  out->drives_reference.gamma_rudder_star_left_rad  =  kRudderBrakeAngleRad;
  out->drives_reference.gamma_rudder_star_right_rad = -kRudderBrakeAngleRad;
  
  // Turn the sail into the flag position, if the wind direction is known.
  double gamma_sail_rad = 0;
  if (filtered.angle_app != kUnknown)
    gamma_sail_rad = SymmetricRad(filtered.angle_app - M_PI);
  out->drives_reference.gamma_sail_star_rad = gamma_sail_rad;

  if (debug) fprintf(stderr, "BrakeController::Run: rudder braked and sail %lf\n", Rad2Deg(gamma_sail_rad));

}
