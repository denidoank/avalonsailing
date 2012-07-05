// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#include "helmsman/docking_controller.h"  

extern int debug;

DockingController::~DockingController() {}

void DockingController::Run(const ControllerInput& in,
                            const FilteredMeasurements& filtered,
                            ControllerOutput* out) {
  out->Reset();
  out->drives_reference.gamma_rudder_star_left_rad  = 0;
  out->drives_reference.gamma_rudder_star_right_rad = 0;
  out->drives_reference.gamma_sail_star_rad = 0;

  if (debug) fprintf(stderr, "DockingController::Run: everything zero\n");
}
