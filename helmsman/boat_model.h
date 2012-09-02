// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// A very rough physical boat model.
#ifndef HELMSMAN_BOAT_MODEL_H
#define HELMSMAN_BOAT_MODEL_H


#include "common/apparent.h"
#include "common/check.h"
#include "common/convert.h"
#include "common/polar.h"
#include "common/sign.h"

#include "helmsman/boat.h" 
#include "helmsman/controller_io.h"

enum Location {
  kBrest,
  kSukku
};


class BoatModel {
 public:
  // everything in radians and meter per second.
  BoatModel(double sampling_period,
            double omega_ = 0,
            double phi_z_ = 0, 
            double v_x_ = 0.,
            double gamma_sail_ = 0.2,
            double gamma_rudder_left_ = 0.0,
            double gamma_rudder_right_ = 0.0,
            Location = kBrest);
  void Simulate(const DriveReferenceValuesRad& drives_reference, 
                Polar true_wind,
                ControllerInput* in);
  void Print(double t);
  void PrintHeader();
  void PrintLatLon(double t);
  
  void SetSpeed(double speed);
  void SetPhiZ(double  phi_z);
  void SetOmega(double omega);
  void SetLatLon(double lat, double lon);
  double GetSpeed();
  double GetPhiZ();

 private:
  // The x-component of the sail force, very roughly.
  double ForceSail(Polar sail_wind_angle, double gamma_sail);
 
  double Saturate(double x, double limit); 
  void FollowRateLimited(double in, double max_rate, double* follows);
  void FollowRateLimitedRadWrap(double in, double max_rate, double* follows);

  void SimDrives(const DriveReferenceValuesRad& drives_reference,
                 DriveActualValuesRad* drives);
  void SetStartPoint(Location start_location);

  // More precise and stable rudder force model
  // We had problems with the very simple one during simulations
  // due to the feedback effect and the simple Euler intergration model.
  void RudderModel(double omega,
                   double period,
                   double* delta_omega_rudder,
                   double* force_rudder_x);
  // Trapez integration model in respect to omega.
  void IntegrateRudderModel(double* delta_omega_rudder, double* force_rudder_x);

  double ForceLift(double alpha_aoa_rad, double speed);
  double ForceDrag(double alpha_aoa_rad, double speed);
  void OneRudder(double gamma_rudder, double v_rudder_alpha,
                 double v_rudder_mag,
                 double* force_x, double* force_y);

  double period_;
  double omega_;
  double phi_z_; 
  double v_x_;

  double gamma_sail_;
  double gamma_rudder_left_;
  double gamma_rudder_right_;
  bool homed_left_;
  bool homed_right_;
  
  double north_deg_;
  double east_deg_;
  double x_;
  double y_;
  
  Polar apparent_;  // apparent wind in the boat frame

  int gps_out_count_;

};
#endif  // HELMSMAN_BOAT_MODEL_H
