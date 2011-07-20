// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011


#include "helmsman/sail_controller.h"

#include <math.h>
#include <stdio.h>

#include "common/check.h"
#include "common/sign.h"
#include "common/normalize.h"

#include "helmsman/sail_controller_const.h"
#include "helmsman/sampling_period.h"  // kSamplingPeriod

extern int debug;

SailModeLogic::SailModeLogic()
    : mode_(WING),
      delay_counter_(0)
  {}

SailMode SailModeLogic::BestMode(double apparent) const {
  CHECK_GE(apparent, 0);
  return apparent < kSwitchpoint ? SPINNAKER : WING;
}

SailMode SailModeLogic::BestStabilizedMode(double apparent) {
  const int delay = static_cast<int>(kSwitchBackDelay / kSamplingPeriod + 0.5);
  if (mode_ == WING_LOCKED) {
    return WING_LOCKED;
  }
  if (mode_ == WING) {
    if (apparent <= kSwitchpoint - 2 * kHalfHysteresis ||
        (apparent < kSwitchpoint - kHalfHysteresis &&
         ++delay_counter_ > delay)) {
      mode_ = SPINNAKER;
      delay_counter_ = 0;
      if (debug) fprintf(stderr, "SailModeLogic::BestMode: Switched to spinnaker.\n"); 
    }
  } else {  // SPINNAKER
    if (apparent > kSwitchpoint + 2 * kHalfHysteresis ||
        (apparent > kSwitchpoint + kHalfHysteresis &&
         ++delay_counter_ > delay)) {
      mode_ = WING;
      delay_counter_ = 0;
      if (debug) fprintf(stderr, "SailModeLogic::BestMode: Switched to wing."); 
    }
  }
  return mode_;
}

void SailModeLogic::LockInWingMode() {
  mode_ = WING_LOCKED;
}

void SailModeLogic::UnlockMode() {
  mode_ = WING;
}

void SailModeLogic::Reset() {
  mode_ = WING;
  delay_counter_ = 0;
}


SailController::SailController()
    // The optimal angle of attack for the trimmed sail, subject to
    // optimization, 10 - 25 degrees.
    : optimal_angle_of_attack_rad_(Deg2Rad(10)),  // degrees.
      sign_(1)  {}

void SailController::SetOptimalAngleOfAttack(double optimal_angle_of_attack_rad) {
  optimal_angle_of_attack_rad_ = optimal_angle_of_attack_rad;
}

double SailController::BestGammaSail(double alpha_wind_rad, double mag_wind) {
  return GammaSailInternal(alpha_wind_rad, mag_wind, false);
}

double SailController::BestStabilizedGammaSail(double alpha_wind_rad, double mag_wind) {
  return GammaSailInternal(alpha_wind_rad, mag_wind, true);
}

double SailController::GammaSailInternal(double alpha_wind_rad,
                                         double mag_wind,
                                         bool stabilized) {
  assert(alpha_wind_rad < 10);
  assert(-alpha_wind_rad > -10);
  alpha_wind_rad = SymmetricRad(alpha_wind_rad);
  alpha_wind_rad = HandleSign(alpha_wind_rad, stabilized);
  CHECK_LE(alpha_wind_rad, M_PI);  // in [0, pi]

  // other lower limit, to avoid unnecessary sail motor activity at low winds?
  if (mag_wind == 0) 
    return 0;

  SailMode mode = stabilized ?
      logic_.BestStabilizedMode(alpha_wind_rad) :
      logic_.BestMode(alpha_wind_rad);

  double gamma_sail_rad = (mode == WING) ?
      (alpha_wind_rad - M_PI + optimal_angle_of_attack_rad_) :
      (0.5 * alpha_wind_rad - kDragMax);      // SPINNAKER mode, broad reach

  return SymmetricRad(sign_ * gamma_sail_rad);
}

void SailController::LockInWingMode() {
  logic_.LockInWingMode();
}

void SailController::UnlockMode() {
  logic_.UnlockMode();
}


// For Stabilized results make a hysteresis around 0.
// For non-stabilized operation make no hysteresis
double SailController::HandleSign(double alpha_wind_rad, bool stabilized) {
  if (stabilized) {
    if (sign_ * alpha_wind_rad < -kHalfHysteresisSign) {
      sign_ = SignNotZero(alpha_wind_rad);
    }
  } else {
    sign_ = SignNotZero(alpha_wind_rad);
  }
  return sign_ * alpha_wind_rad;  
}

void SailController::Reset() {
  logic_.Reset();
  optimal_angle_of_attack_rad_ = Deg2Rad(10);
  sign_ = 1;
}

