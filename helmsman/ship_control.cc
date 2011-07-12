// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/ship_control.h"

#include <stdio.h>
#include "lib/fm/log.h"

MetaState ShipControl::meta_state_ = kNormal;

WindStrengthRange ShipControl::wind_strength_ = kCalmWind;
WindStrengthRange ShipControl::wind_strength_apparent_ = kCalmWind;

RudderController ShipControl::rudder_controller_;
SailController ShipControl::sail_controller_;

InitialController ShipControl::initial_controller_(
    &ShipControl::sail_controller_);
BrakeController   ShipControl::brake_controller_;
DockingController ShipControl::docking_controller_;
NormalController  ShipControl::normal_controller_(
    &ShipControl::rudder_controller_,
    &ShipControl::sail_controller_);

Controller* ShipControl::controller_ = &ShipControl::initial_controller_; 
FilteredMeasurements ShipControl::filtered_;
FilterBlock* ShipControl::filter_block_ = new FilterBlock;

// All methods are static.
void ShipControl::Transition(Controller* new_state,
                             const ControllerInput& in) {
  FM_LOG_INFO("Leaving %s", controller_->Name());
  controller_->Exit();
  controller_ = new_state;
  FM_LOG_INFO("Entering %s", controller_->Name());
  printf("Entering %s\n", controller_->Name());
  controller_->Entry(in, filtered_);  
}

bool ShipControl::Brake() {
  meta_state_ = kBraking;
  return true;
}

bool ShipControl::Docking() {
  meta_state_ = kDocking;
  return true;
}


bool ShipControl::Normal() {
  meta_state_ = kNormal;
  return true;
}
/*
bool ShipControl::OverrideSkipper(const char* dummy_command) {
  meta_state = kNormal;
  alpha_star = read value from command
  return true;
}
*/

void ShipControl::StateMachine(const ControllerInput& in) {
  // Meta-States
  if (controller_ != &brake_controller_ && meta_state_ == kBraking) {
    Transition(&brake_controller_, in);
    return;
  }  
  if (controller_ != &docking_controller_ && meta_state_ == kDocking) {
    Transition(&docking_controller_, in);
    return;
  }  
  if ((controller_ == &brake_controller_ ||
       controller_ == &docking_controller_) &&
       meta_state_ == kNormal) {
    Transition(&initial_controller_, in);
    return;
  }  
  if (meta_state_ != kNormal)
    return;

  // Normal states
  // Fallback to initial controller state
  if (controller_ != &initial_controller_) {
    if (!in.drives.homed_sail ||
        (!in.drives.homed_rudder_left && !in.drives.homed_rudder_right)) {
      Transition(&initial_controller_, in);
      return;
    }
  }

  // all other transition decisions, possibly delegated to the controllers
  if (controller_ == &initial_controller_) {
    /*
    if (controller_.Done() && wind_strength_apparent_ == kStormWind) {
      Transition(&normal_to_storm_controller_, in);
      return;
    }
    */
    if (controller_->Done() &&
        wind_strength_ == kNormalWind &&
        filter_block_->ValidTrueWind() &&
        in.alpha_star_rad != kUnknown) {
      Transition(&normal_controller_, in);
      return;
    } else {
      static int log_count = 0;
      if (++log_count % 10 == 0) {
        if (!controller_->Done())                  
          FM_LOG_INFO("Initial not Done.");
        else if (!wind_strength_ == kNormalWind)   
          FM_LOG_INFO("Wind not normal");
        else if (!filter_block_->ValidTrueWind())  
          FM_LOG_INFO("True wind filter not valid");
        else if (!in.alpha_star_rad != kUnknown) 
          FM_LOG_INFO("alpha_star unknown"); 
      }  
    }
  }
}

// This needs to run with the sampling period.
bool ShipControl::Run(const ControllerInput& in, ControllerOutput* out) {
  ControllerOutput prev_out = *out;
  // Get wind speed and all other actual measurement values
  // Figure out apparent and true wind
  filter_block_->Filter(in, &filtered_);
  if (filter_block_->ValidTrueWind()) {
    wind_strength_ = WindStrength(wind_strength_, filtered_.mag_true);
    out->skipper_input.longitude_deg = filtered_.longitude_deg;
    out->skipper_input.latitude_deg =  filtered_.latitude_deg;
    out->skipper_input.angle_true_deg = NormalizeDeg(Rad2Deg(filtered_.alpha_true));
    out->skipper_input.mag_true_kn =    MeterPerSecondToKnots(filtered_.mag_true);
  }  
  wind_strength_apparent_ =
      WindStrength(wind_strength_apparent_, filtered_.mag_app);

  // TODO fill skipper data

  // find state 
  StateMachine(in);
  // call specialized controller
  controller_->Run(in, filtered_, out);
  
  
  bool changed = prev_out != *out;
  prev_out = *out;
  return changed;
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

