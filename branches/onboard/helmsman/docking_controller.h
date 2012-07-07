// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_DOCKING_CONTROLLER_H
#define HELMSMAN_DOCKING_CONTROLLER_H

#include "controller.h"

class DockingController : public Controller {
 public:
  virtual ~DockingController();
  virtual void Run(const ControllerInput& in,
                   const FilteredMeasurements& filtered,
                   ControllerOutput* out);
  virtual const char* Name() {
    return "Docking";
  };
};

#endif  // HELMSMAN_DOCKING_CONTROLLER_H
