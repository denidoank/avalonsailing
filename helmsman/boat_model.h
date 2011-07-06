// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// A very rough physical boat model.
#ifndef HELMSMAN_BOAT_MODEL_H
#define HELMSMAN_BOAT_MODEL_H


#include "common/check.h"
#include "common/convert.h"
#include "common/polar.h"
#include "common/sign.h"

#include "helmsman/apparent.h"
#include "helmsman/boat.h" 
#include "helmsman/controller_io.h"

class BoatModel {
 public:
  // everything in radians and meter per second.
  BoatModel(double sampling_period,
            double omega_ = 0,
            double phi_z_ = 0, 
            double v_x_ = 0.,
            double gamma_sail_ = 0.2,
            double gamma_rudder_left_ = 0.03,
            double gamma_rudder_right_ = -0.05);
  // N.B. braking is not simulated correctly!
  // (The drag force is not considered in the rudder force model)
  void Simulate(const DriveReferenceValuesRad& drives_reference, 
                Polar true_wind,
                ControllerInput* in);
  void Print(double t);
  void PrintHeader();
  
  void SetSpeed(double speed);
  void SetPhiZ(double  phi_z);
  void SetOmega(double omega);
  double GetSpeed();
  double GetPhiZ();

 private:
  // The x-component of the sail force, very roughly.
  double ForceSail(Polar sail_wind_angle, double gamma_sail);
  double RudderAcc(double gamma_rudder, double water_speed);
 
  double Saturate(double x, double limit); 
  void FollowRateLimited(double in, double max_rate, double* follows);
  void FollowRateLimitedRadWrap(double in, double max_rate, double* follows);

  void SimDrives(const DriveReferenceValuesRad& drives_reference,
                 DriveActualValuesRad* drives);

  double period_;
  double omega_;
  double phi_z_; 
  double v_x_;

  double gamma_sail_;
  double gamma_rudder_left_;
  double gamma_rudder_right_;
  int homing_counter_left_;
  int homing_counter_right_;
  
  double north_;
  double east_;
  Polar apparent_;  // apparent wind in the boat frame
};
#endif  // HELMSMAN_BOAT_MODEL_H
