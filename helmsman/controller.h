// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_CONTROLLER_H
#define HELMSMAN_CONTROLLER_H

#include "helmsman/controller_io.h"


class Controller {
 public:
  virtual ~Controller();

  // with default implementaton here.
  // Entry needs to update all variable states!
  virtual void Entry(const ControllerInput& in,
                     const FilteredMeasurements& filtered);
  virtual void Run(const ControllerInput& in,
                   const FilteredMeasurements& filtered,
                   ControllerOutput* out) = 0;
  // with default implementaton here
  virtual void Exit();
  virtual bool Done();
  virtual const char* Name() = 0;
};


#endif  // HELMSMAN_CONTROLLER_H
