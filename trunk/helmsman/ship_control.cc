// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/ship_control.h"

#include "lib/fm/log.h"

MetaState ShipControl::meta_state_ = kNormal;

InitialController ShipControl::initial_controller_;
BrakeController   ShipControl::brake_controller_;
DockingController ShipControl::docking_controller_;

Controller* ShipControl::controller_ = &ShipControl::initial_controller_; 
FilteredMeasurements ShipControl::filtered_;
FilterBlock ShipControl::filter_block_;

// All methods are static.
void ShipControl::Transition(Controller* new_state,
                             const ControllerInput& in) {
  FM_LOG_INFO("Leaving %s", controller_->Name());
  controller_->Exit();
  controller_ = new_state;
  FM_LOG_INFO("Entering %s", controller_->Name());
  controller_->Entry(in, filtered_);  
}

bool ShipControl::Brake(const char* dummy_command) {
  meta_state_ = kBraking;
  return true;
}

bool ShipControl::Docking(const char* dummy_command) {
  meta_state_ = kDocking;
  return true;
}


bool ShipControl::Normal(const char* dummy_command) {
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

  if (meta_state_ == kBraking) {
    Transition(&brake_controller_, in);
    return;
  }  

  if (meta_state_ == kDocking) {
    Transition(&docking_controller_, in);
    return;
  }  
  
  
  /*
  if (drives not ready) {
    Transition(&initial_controller_, in);
    return;
  }
  */
  

  // all other transition decisions, possibly delegated to the controllers
  
}

// This needs to run with the sampling period.
void ShipControl::Run(const ControllerInput& in, ControllerOutput* out) {
  // Get wind speed and all other actual measurement values
  // Figure out apparent and true wind
  filter_block_.Filter(in, &filtered_);
  // TODO fill skipper data

  // find state 
  StateMachine(in);
  // call specialized controller
  controller_->Run(in, filtered_, out);
}

