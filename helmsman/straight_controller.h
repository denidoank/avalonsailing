// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_STRAIGHT_CONTROLLER_H
#define HELMSMAN_STRAIGHT_CONTROLLER_H

#include "helmsman/controller.h"

class StraightController : public Controller {
 public:
  virtual ~StraightController();
  virtual void Run(const ControllerInput& in,
                   const FilteredMeasurements& filtered,
                   ControllerOutput* out);
  virtual const char* Name() {
    return "Straight";
  };
};

#endif  // HELMSMAN_STRAIGHT_CONTROLLER_H
