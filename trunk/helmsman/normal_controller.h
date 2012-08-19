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
  double RateLimit();

  // This method is public for test accessibility only.
  void ShapeReferenceValue(double alpha_star,
                           double alpha_true, double mag_true,
                           double phi_z_boat, double mag_boat,
                           double angle_app,  double mag_app,
                           double old_gamma_sail,
                           double* phi_z_star,
                           double* omega_z_star,
                           double* gamma_sail_star,
                           ControllerOutput* out);
  // public for test only.
  double FilterOffset(double offset);

 private:
  // Near for bearings.
  bool Near(double a, double b);
  bool IsJump(double old_direction, double new_direction);
  // The current bearing is near the TackZone (close reach) or near the Jibe Zone (broad reach)
  // and we will have to do a maneuver.
  bool IsGoodForManeuver(double old_direction, double new_direction, double angle_true);

  bool OutputChanges(const DriveReferenceValuesRad& out,
                     double gamma_rudder_star,
                     double gamma_sail_star);

  double NowSeconds();                   

  RudderController* rudder_controller_;
  SailController* sail_controller_;
  // alpha* is rate limited.
  double alpha_star_rate_limit_;
  double old_phi_z_star_;  // keeps the last shaped alpha*, needed for maneuver distinction.
  ReferenceValues ref_;    // Has state about running maneuvers.
  int give_up_counter_;    // delays fallback to the InitialController
  int64_t start_time_ms_;
  int trap2_;  // paranoid protection against incomplete compilation errors.
  double prev_offset_;
  double fallen_off_;
  ManeuverType maneuver_type_;
};

#endif  // HELMSMAN_NORMAL_CONTROLLER_H
