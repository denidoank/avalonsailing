// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// No assumptions about the availablility of a alpha_star (desired heading) 
// here, because the GPS/IMU might not be able to give a correct direction
// information. The Initial controller tries to sail downwind. It does not
// follow the skipper direction alpha_star.
// When the initial controller state is left, 
//   1. we have covered some distance (so the IMU should be capable of getting
//      speed and direction info),
//   2. the true wind filter is filled,
//   3. the boat has some forward speed and the rudders have the usual effect. 

#ifndef HELMSMAN_INITIAL_CONTROLLER_H
#define HELMSMAN_INITIAL_CONTROLLER_H

#include "helmsman/controller.h"
#include "helmsman/sail_controller.h"

class InitialController : public Controller {
 public:
  InitialController(SailController* sail_controller);
  virtual ~InitialController();
  virtual void Entry(const ControllerInput& in,
                     const FilteredMeasurements& filtered);
  virtual void Run(const ControllerInput& in,
                   const FilteredMeasurements& filtered,
                   ControllerOutput* out);
  virtual bool Done();
  virtual void Exit();
  virtual const char* Name() {
    return "Initial";
  };
  
 private:
  // Phase sleep serves to settle the filters.
  // Phase Turtle means to turn until we are in a sailable direction,
  //   if necessary and to wait for enough wind (WindStrength).
  // Phase KOGGE means to sail from the wind, i.e. our direction is determined
  //   by the wind direction
  enum Phase {
    SLEEP,
    TURTLE,
    KOGGE
  };
  
  void Reset();
  SailController* sail_controller_;
  Phase phase_;
  // sign_ = 1 means: Turn to starboard, sail away to starboard,
  // sign_ = -1 means: Turn to portside, sail away to portside.
  int sign_; 
  int count_;
  bool positive_speed_;
};

#endif  // HELMSMAN_INITIAL_CONTROLLER_H
