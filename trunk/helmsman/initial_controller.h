// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_INITIAL_CONTROLLER_H
#define HELMSMAN_INITIAL_CONTROLLER_H

#include "helmsman/controller.h"

class InitialController : public Controller {
 public:
  virtual ~InitialController();
  virtual void Entry(const ControllerInput& in,
                     const FilteredMeasurements& filtered);
  virtual void Run(const ControllerInput& in,
                   const FilteredMeasurements& filtered,
                   ControllerOutput* out);
  virtual void Exit();
  virtual const char* Name() {
    return "Initial";
  };
};

#endif  // HELMSMAN_INITIAL_CONTROLLER_H
