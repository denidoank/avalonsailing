// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/initial_controller.h"  


void InitialController::Run(const ControllerInput& in,
                            const FilteredMeasurements& filtered,
                            ControllerOutput* out) {
  out->Reset();
  
  // Turn the sail into the flag position, if the wind direction is known.
  //double gamma_sail_rad = 0;
  //if (filtered.alpha_apparent != kUnknown)
  //  gamma_sail_rad = filtered.alpha_apparent - M_PI;
  //out->drive_reference_values.gamma_sail_star_deg = Rad2Deg(gamma_sail_rad);
}

InitialController::~InitialController() {}

void InitialController::Entry(const ControllerInput& in,
                              const FilteredMeasurements& filtered) {}
void InitialController::Exit() {}

