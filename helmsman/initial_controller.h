// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_INITIAL_CONTROLLER_H
#define HELMSMAN_INITIAL_CONTROLLER_H

#include "helmsman/controller.h"

class InitialController : public Controller {
 public:
  InitialController();
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
 private:
  // Phase sleep serves to settle the filters.
  // Phase Turtle means to turn until we are in a sailable direction, if necessary
  // Phase KOGGE means to sail from the wind, i.e. our direction is determined by
  //   the wind direction
  enum Phase {
    SLEEP,
    TURTLE,
    KOGGE
  };
  
  void Reset();
  void PhaseChoice(double angle_attack);
  
  Phase phase_;
  int gamma_sign_; 
  int count_;
};

#endif  // HELMSMAN_INITIAL_CONTROLLER_H
