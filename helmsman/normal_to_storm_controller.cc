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

/*
#include "helmsman/normal_to_storm_controller.h"


#include "common/delta_angle.h"

//#include "common/unknown.h"       // kUnknown
#include "common/sign.h"
#include "helmsman/controller.h"
#include "helmsman/storm_controller_const.h"
#include "helmsman/brake_controller.h" // ?? need sign_gamma_sail_

// We turn slowly.
const double kTurnRateRad = M_PI / 10;  // rad/s, 10s for 180 degrees


void NormalToStormController::Entry(const ControllerInput& in,
                                    const FilteredMeasurements& filtered) {
  sail_controller_.LockInWingMode();
  storm_heading_ = NearestRad(in.alpha_star_rad,
                              filtered.wind_true_alpha - kBoatStormAngle,
                              filtered.wind_true_alpha - kBoatStormAngle);

  alpha_star_ = filtered.phi_z_rad;
  double turn = DeltaRad(alpha_star_, storm_heading_);
  delta_alpha_star_ = Sign(turn) * kTurnRateRad * kSamplingPeriod
  duration_ = turn / delta_alpha_star;
  done_ = false;
}

void NormalToStormController::Run(const ControllerInput& in,
                                  const FilteredMeasurements& filtered,
                                  ControllerOutput* out) {
  alpha_star += delta_alpha_star_;
  if (duration > 0)
    --duration_;
  else
    done_ = true;

  out->Reset();
  out->drive_reference.gamma_sail_rad =
      sail_controller_.BestGammaSail(filtered.apparent_rad,
                                     filtered.apparent_mag);
  double gamma_rudder = heading_controller.Control(alpha_star, in, filtered);
  out->drive_reference.gamma_rudder_left_rad = gamma_rudder;
  out->drive_reference.gamma_rudder_right_rad = gamma_rudder;
}

void NormalToStormController::Exit() {}

bool NormalToStormController::Done() {
  return done_;
}







bool ActiveState(Controller* state_controller) {
  return state_controller == controller;
}

if (ActiveState(normal_to_storm_controller_)) {
  if (normal_to_storm_controller_.Done())
    Transition(&storm_controller, in_);
}

if (ActiveState(storm_controller_)) {
  if (Windtrength(in_) == kNormal)
    Transition(&normal_controller, in_);
}



*/

