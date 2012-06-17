// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/ship_control.h"

#include <stdio.h>

extern int debug; // globally shared

MetaState ShipControl::meta_state_  = kNormal;
WindStrengthRange ShipControl::wind_strength_          = kCalmWind;
WindStrengthRange ShipControl::wind_strength_apparent_ = kCalmWind;

RudderController ShipControl::rudder_controller_;
SailController   ShipControl::sail_controller_;

InitialController ShipControl::initial_controller_(&ShipControl::sail_controller_);
BrakeController   ShipControl::brake_controller_;
DockingController ShipControl::docking_controller_;
IdleController    ShipControl::idle_controller_;
NormalController  ShipControl::normal_controller_(&ShipControl::rudder_controller_, &ShipControl::sail_controller_);
TestController    ShipControl::test_controller_(&ShipControl::sail_controller_);

// This is the state of the ship controller state machine.
Controller* ShipControl::controller_ = &ShipControl::test_controller_;  // for drive and sensor tests
// Controller* ShipControl::controller_ = &ShipControl::initial_controller_; // for fast startup and Atlantic

FilteredMeasurements ShipControl::filtered_;
FilterBlock* ShipControl::filter_block_ = new FilterBlock;

// All methods are static.
void ShipControl::Transition(Controller* new_state, const ControllerInput& in) {
  if (debug)
    fprintf(stderr,"Transition %s -> %s\n", controller_->Name(), new_state->Name());
  controller_->Exit();
  controller_ = new_state;
  controller_->Entry(in, filtered_);  
}

void ShipControl::StateMachine(const ControllerInput& in) {
  if (debug && 0)
    fprintf(stderr, "Entering Statemachine with %s\n", controller_->Name());

  switch (meta_state_) {
  case kBraking:
    if (controller_ != &brake_controller_) {
      Transition(&brake_controller_, in);
    }
    return;

  case kDocking:
    if (controller_ != &docking_controller_) {
     Transition(&docking_controller_, in);
    }  
    return;

  case kIdle:
    if (controller_ != &idle_controller_) {
     Transition(&idle_controller_, in);
    }
    return;

  case kNormal:
    if (controller_ != &initial_controller_ &&
        controller_ != &normal_controller_ &&
        controller_ != &test_controller_) {
      Transition(&initial_controller_, in);
    }
    // no return
  }

  // Normal states
  if (controller_ != &initial_controller_ && controller_ != &test_controller_) {
    if (!in.drives.homed_sail || (!in.drives.homed_rudder_left && !in.drives.homed_rudder_right)) {
      Transition(&initial_controller_, in);
      return;
    }
  }

  if (controller_ == &test_controller_) {
    if (controller_->Done())
      Transition(&initial_controller_, in);
  }

  // all other transition decisions, possibly delegated to the controllers
  if (controller_ == &initial_controller_) {
    if (debug)
      fprintf(stderr, "Initial controller %sdone wind strenght:%s wind valid:%s alpha_star:%6.4lf deg\n",
	      controller_->Done() ? "" : "NOT ",
	      WindToString(wind_strength_apparent_),
	      filter_block_->ValidTrueWind() ? "YES" : "NO",
	      in.alpha_star_rad != kUnknown ? Rad2Deg(in.alpha_star_rad) : NAN);

    if (controller_->Done() &&
        wind_strength_apparent_ != kCalmWind &&
        filter_block_->ValidTrueWind() &&
        in.alpha_star_rad != kUnknown) {
      Transition(&normal_controller_, in);
      return;
    } 
    if (debug)
      fprintf(stderr, "InitialState, wait for true wind info and alpha_star\n");
  }

  if (controller_ == &normal_controller_) {
    if (normal_controller_.GiveUp(in, filtered_)) {
      // Assert here to find situations in which the boat gets stuck in tests.
      // assert(0);
      Transition(&initial_controller_, in);
      return;
    }
  }

  if (debug && 0)
    fprintf(stderr, "ShipControl::StateMachine stationary in state %s\n", controller_->Name());
}

// This needs to run with the sampling period of 100ms.
void ShipControl::Run(const ControllerInput& in, ControllerOutput* out) {
  ControllerOutput prev_out = *out;

  // Get wind speed and all other actual measurement values.
  // Figure out apparent and true wind.
  filter_block_->Filter(in, &filtered_);

  // Input for the Skipper
  if (filter_block_->ValidTrueWind()) {
    wind_strength_                     = WindStrength(wind_strength_, filtered_.mag_true);
    out->skipper_input.longitude_deg   = filtered_.longitude_deg;
    out->skipper_input.latitude_deg    = filtered_.latitude_deg;
    out->skipper_input.angle_true_deg  = NormalizeDeg(Rad2Deg(filtered_.alpha_true));
    out->skipper_input.mag_true_kn     = MeterPerSecondToKnots(filtered_.mag_true);
    out->status.direction_true_deg     = NormalizeDeg(Rad2Deg(filtered_.alpha_true));
    out->status.mag_true_m_s           = filtered_.mag_true;
  }  

  wind_strength_apparent_ = WindStrength(wind_strength_apparent_, filtered_.mag_app);

  // Find state 
  StateMachine(in);
  // Call specialized controller
  controller_->Run(in, filtered_, out);
}

// Needed for tests only
void ShipControl::Reset() {
  delete filter_block_;
  filter_block_ = new FilterBlock;
  wind_strength_ = kCalmWind;
  wind_strength_apparent_ = kCalmWind;
  rudder_controller_.Reset();
  sail_controller_.Reset();
  controller_ = &initial_controller_; 
}

bool ShipControl::Idling() {
  return(controller_ == &idle_controller_);
}
