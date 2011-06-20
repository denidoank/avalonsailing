// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// A controller for the normal to storm transition
// Initial conditions:
// 1. The boat is heading somewhere into the sailable region
//    i.e. true wind vector in [-180 deg + kTackZone, 180 deg - TackZone]
//    Due to disturbance forces we might be in this zone anyway.
//    TODO(grundmann): Make sure that transitions to storm mode cannot
//    start during a tack.
// 2. The wind speed is sufficienly high, thus the boat speed is above 2 knots
//    (4m/s) and the boats heading is well controllable. 
// Final conditions:
// * heading angle (reference value to the heading controller) 
//   alpha_star == true wind vector direction +- kBoatStormAngle
//     i.e. apparent wind vector direction = +- kBoatStormAngle.
//     That direction which is nearer to the desired skipper heading is chosen.
//     For this decision the initial alpha_star is relevant, i.e. during the turn
//     changes of the desired heading are ignored.
// 
// transition phases
// 0. decide which way to turn, prepare
// 1. lock sail controller in wing mode
// 2. new heading = alpha_true +- kBoatStormAngle, transit with constant turning rate omega_z
//    need fast apparent wind filter for that such that the sail angle can follow (or feed forward
// int gamma_sail_sign = sign (initial gamma_sail);
// set constant gamma_sail, brake the sail drive (done by drive control)

// TODO(grundmann): on exit from Storm mode unlock the sail controller
#ifndef HELMSMAN_NORMAL_TO_STORM_CONTROLLER_H
#define HELMSMAN_NORMAL_TO_STORM_CONTROLLER_H

#include "helmsman/controller.h"

class NormalToStormController {
 public:
  virtual void Entry(const ControllerInput& in,
                     const FilteredMeasurements& filtered);
  virtual void Run(const ControllerInput& in,
                   const FilteredMeasurements& filtered,
                   ControllerOutput* out);
  virtual void Exit();
  bool Done();
 private: 
  SailController sail_controller_; 
};

#endif  // HELMSMAN_NORMAL_TO_STORM_CONTROLLER_H
