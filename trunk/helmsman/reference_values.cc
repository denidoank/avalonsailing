// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

// Make synchronized reference angles for tack, jibe and change maneuvers.
// If we sail closed reach and we sail a tack then we first tack to the other
// closed reach., stabilize for 2 seconds and then turn further if necessary.
// The maneuvers are calculated with overshoot, but limited to the minimal
// maneuver angle.

#include "helmsman/reference_values.h"

#include <algorithm>  // max

#include <math.h>
#include <stdio.h>
#include "common/check.h"
#include "common/convert.h"
#include "common/delta_angle.h"
#include "common/normalize.h"
#include "helmsman/boat.h"  // constants from simulation/boat.m, kMaxOmegaSail
#include "helmsman/sampling_period.h"

extern int debug;

// General minimal time for a turn, in s
const static double kDurationNormal = 4;
// A tack changes the apparent wind angle and that angle measurement is
// filtered and delayed, such that the sail angle calculated from it is
// temporarily incorrect. When the stabilization passed then the
// apparent wind angle is has become correct and we follow it,
// as usual.
// After a tack (or other maneuver) the sail angle is held stable for this time.
const static double kStabilizationPeriod = 1.2;

ReferenceValues::ReferenceValues()
    : tick_(1E6),
      all_ticks_(0),
      stabilization_ticks_((kStabilizationPeriod + kSamplingPeriod / 2) / kSamplingPeriod),
      acc_(0.0),
      phi_z_(0.0),
      gamma_sail_(0.0),
      omega_(0.0),
      omega_sail_increment_(0.0),
      phi_z_final_(0.0),
      gamma_sail_final_(0.0) {}

void ReferenceValues::SetReferenceValues(double phi_z_star,
                                         double gamma_sail_star) {
  phi_z_final_ = phi_z_star;
  phi_z_ = phi_z_star;
  gamma_sail_final_ = gamma_sail_star;
  gamma_sail_ = gamma_sail_star;
  all_ticks_ = 0;
  if (debug) fprintf(stderr, "SetRef:heading %6.2lf deg Sail:%6.2lf deg\n", Rad2Deg(phi_z_), Rad2Deg(gamma_sail_));
}

// Do a turn of the boat (a normal change of direction, a tack or a jibe)
// according to this plan:
// 1. Accelerate for 1/6 of the time,
// 2. keep the rotational speed
//    constant for 2/3 of the total time T and
// 3. finally decelerate for 1/6 of the time.
// Make a feasible plan that allows to turn the sail during the turn and
// is feasible with the current rudder forces (speed dependant).
void ReferenceValues::NewPlan(double phi_z_1,
                              double delta_gamma_sail,  // gamma sail angles are not normalized
                              double speed) {
  phi_z_1 = SymmetricRad(phi_z_1);
  phi_z_final_ = phi_z_1;
  gamma_sail_final_ = gamma_sail_ + delta_gamma_sail;  // intentionally no normalization here, because the sails way may be longer than 180 degrees
  double delta_phi = DeltaOldNewRad(phi_z_, phi_z_1);
  omega_ = 0;
  // Turn around such that 3 conditions are met:
  // * The sail can follow.
  double duration_sail = fabs(delta_gamma_sail) / kOmegaMaxSail;

  // * The rudders don't stall.
  // maximum rotational acceleration is speed dependant.
  double acc_max = 0.25 * speed * speed;  // rad/s^2
  acc_max = std::max(acc_max, 0.1);  // lower limit to limit the duration
  // Minimum time for the turn due to acceleration limit.
  double duration_acc = sqrt(fabs(delta_phi) / acc_max * 36.0 / 5);
  // * It takes at least kDurationNormal seconds.
  double duration = std::max(std::max(kDurationNormal, duration_sail),
                             duration_acc);
  // round duration up to next multiple of 6 sampling periods
  // ticks_ is the number of calls spent in the first phase of the plan.
  int ticks = ceil(duration / (6 * kSamplingPeriod));
  duration = ticks * 6 * kSamplingPeriod;
  all_ticks_ = 6 * ticks;

  // correct acc_max
  acc_ = delta_phi * 36.0 / (5 * duration * duration);

  omega_sail_increment_ = delta_gamma_sail / all_ticks_;
  tick_ = 0;
  if (debug) {
    fprintf(stderr, "New Plan: delta_phi: %6.4lf deg\n", Rad2Deg(delta_phi));
    fprintf(stderr, "delta_gamma_sail: %6.4lf deg, duration %6.4lf s\n", Rad2Deg(delta_gamma_sail), duration);
    fprintf(stderr, "duration_sail: %6.4lf s, duration_acc: %6.4lf s\n", duration_sail, duration_acc);
    fprintf(stderr, "!!End state: phi_z %6.4lf, gamma_sail: %6.4lf\n", phi_z_final_, gamma_sail_final_);
  }
}

bool ReferenceValues::RunningPlan() {
  return tick_ < all_ticks_ + stabilization_ticks_;
}

void ReferenceValues::GetReferenceValues(double* phi_z_star, double* omega_z_star, double* gamma_sail_star) {
  if (tick_ >= all_ticks_ + stabilization_ticks_) {
    phi_z_ = phi_z_final_;  // This eliminates accumulated errors in the frequent floating point additions.
    gamma_sail_ = SymmetricRad(gamma_sail_final_);
    *phi_z_star = phi_z_;
    *omega_z_star = 0;
    *gamma_sail_star = gamma_sail_;
    return;
  }

  double a;
  if (tick_ < all_ticks_ / 6) {
    a = acc_;
  } else if (tick_ < all_ticks_ * 5 / 6) {
    a = 0;
  } else if (tick_ < all_ticks_) {
    a = -acc_;
  } else if (tick_ < all_ticks_ + stabilization_ticks_) {
    a = 0;
    omega_ = 0;
    omega_sail_increment_ = 0;
  } else {
    CHECK(0);  // no RunningPlan
  }
  omega_ += a * kSamplingPeriod;
  phi_z_ += omega_ * kSamplingPeriod;
  phi_z_ = SymmetricRad(phi_z_);
  gamma_sail_ += omega_sail_increment_;
  if (tick_ < 1E6)
    ++tick_;

  *phi_z_star = phi_z_;
  *omega_z_star = omega_;
  *gamma_sail_star = SymmetricRad(gamma_sail_);
}
