// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_BRAKE_CONTROLLER_H
#define HELMSMAN_BRAKE_CONTROLLER_H

#include "helmsman/controller.h"

// BrakeController
//
// Simple-minded controller for stopping the boat in emergency situations e.g.
// during test.
// Initial state: All states are possible
// Final state: Rudders at +- 80 degrees, sail has angle of attack = 0 to
// the apparent wind in order to produce as little as possible force.

static const double kRudderBrakeAngleRad = Deg2Rad(60);

class BrakeController : public Controller {
 public:
  virtual ~BrakeController();
  virtual void Entry(const ControllerInput& in,
                     const FilteredMeasurements& filtered);
  virtual void Run(const ControllerInput& in,
                   const FilteredMeasurements& filtered,
                   ControllerOutput* out);
  virtual const char* Name() {
    return "Brake";
  };
 private:
  int sign_;
  int count_;
  double gamma_sail_rad_;
};

#endif  // HELMSMAN_BRAKE_CONTROLLER_H
