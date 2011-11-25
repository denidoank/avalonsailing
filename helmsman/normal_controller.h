// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// This is the controller for normal sailing operation.
// This controller assumes that all sensor information (heading and speed
// especially are available and that the wind is strong enough to measure
// a reliable wind direction and to sustain speed and boat direction
// controllability.
// This controller uses rudder and sail controllers and performs tacks and jibes.
//

#ifndef HELMSMAN_NORMAL_CONTROLLER_H
#define HELMSMAN_NORMAL_CONTROLLER_H

#include "helmsman/controller.h"
#include "helmsman/maneuver_type.h"
#include "helmsman/rudder_controller.h"
#include "helmsman/reference_values.h"
#include "helmsman/sail_controller.h"

class NormalController : public Controller {
 public:
  NormalController(RudderController* rudder_controller,
                   SailController* sail_controller);
  virtual ~NormalController();
  virtual void Entry(const ControllerInput& in,
                     const FilteredMeasurements& filtered);
  virtual void Run(const ControllerInput& in,
                   const FilteredMeasurements& filtered,
                   ControllerOutput* out);
  virtual void Exit();
  virtual const char* Name() {
    return "Normal";
  }

  bool TackingOrJibing();
  // If the wind ceases and the speed drops, we cannot sail
  // and restart with the Initial Controller.
  bool GiveUp(const ControllerInput& in,
              const FilteredMeasurements& filtered);
  // for tests
  void SkipAlphaStarShaping(bool skip);

 private:
  ManeuverType ReferenceValueSwitch(double alpha_star,
                            double alpha_true, double mag_true,
                            double alpha_boat, double mag_boat,
                            double alpha_app,  double mag_app,
                            double old_gamma_sail,
                            double* phi_z_star,
                            double* omega_z_star,
                            double* gamma_sail_star);

  bool IsJump(double old_direction, double new_direction);
  bool ShapeAlphaStar(double alpha_star,
                      double alpha_true_wind,
                      double* alpha_star_restricted);

  bool OutputChanges(const DriveReferenceValuesRad& out,
                     double gamma_rudder_star,
                     double gamma_sail_star);

  double NowSeconds();                   

  RudderController* rudder_controller_;
  SailController* sail_controller_;
  // The desired direction is usually "shaped". See steps 1 and 2 below.
  // For tests shaping can be switched off with SkipAlphaStarShaping(true).
  bool skip_alpha_star_shaping_;
  // 1. The external alpha_star is rate limited.
  double alpha_star_rate_limit_;
  double alpha_star_rate_limited_;
  // 2. alpha_star_rate_limited_ is restricted to the sailable sectors.
  double alpha_star_restricted_;

  double prev_alpha_star_restricted_;  // keeps the last shaped alpha, needed for maneuver distinction.

  ReferenceValues ref_;
  int give_up_counter_;
  uint64_t start_time_ms_;
  int trap2_;
};

#endif  // HELMSMAN_NORMAL_CONTROLLER_H
