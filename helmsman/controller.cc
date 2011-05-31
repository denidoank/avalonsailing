// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#include "helmsman/controller.h"  

Controller::~Controller() {}

void Controller::Entry(const ControllerInput& in,
                       const FilteredMeasurements& filtered) {}

void Controller::Exit() {}

