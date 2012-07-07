// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#include "idle_controller.h"

extern int debug;

IdleController::~IdleController() {}

void IdleController::Run(const ControllerInput& in,
                         const FilteredMeasurements& filtered,
                         ControllerOutput* out) {

  if (debug) fprintf(stderr, "IdleController::Run: No action\n");
}
