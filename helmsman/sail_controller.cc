// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011


#include "helmsman/sail_controller.h"

#include <math.h>

#include "lib/fm/log.h"
#include "common/check.h"

#include "helmsman/sail_controller_const.h"
#include "helmsman/sampling_period.h"  // kSamplingPeriod


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
      FM_LOG_INFO("Switched to spinnaker."); 
    }
  } else {
    if (apparent > kSwitchpoint + 2 * kHalfHysteresis ||
        (apparent > kSwitchpoint + kHalfHysteresis &&
         ++delay_counter_ > delay)) {
      mode_ = WING;
      delay_counter_ = 0;
      FM_LOG_INFO("Switched to wing."); 
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


SailController::SailController()
    // The optimal angle of attack for the trimmed sail, subject to
    // optimization, 10 - 25 degrees.
    : optimal_angle_of_attack_rad_(10/180.0*M_PI)  // degrees. 
  {}

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
  int sign = 1;
  if (alpha_wind_rad < 0) {
    alpha_wind_rad = -alpha_wind_rad;
    sign = -1;
  }
  CHECK_LE(alpha_wind_rad, M_PI);  // in [0, pi]

  // other lower limit, to avoid unnecessary sail motor activity at low winds?
  CHECK_GE(mag_wind, 0);
  double gamma_sail_rad = 0;
  if (mag_wind > 0) {
    SailMode mode = stabilized ?
        logic_.BestStabilizedMode(alpha_wind_rad) :
        logic_.BestMode(alpha_wind_rad);
    if (mode == WING)
      gamma_sail_rad =
          sign * (alpha_wind_rad - M_PI + optimal_angle_of_attack_rad_);
    else  // SPINNAKER mode, broad reach
      gamma_sail_rad = sign * (0.5 * alpha_wind_rad - kDragMax);
  } else {
     gamma_sail_rad = 0; // std::cout << "Niy : no wind, mag_wind:" << mag_wind << "\n";
  }
  return gamma_sail_rad;
}

void SailController::LockInWingMode() {
  logic_.LockInWingMode();
}

void SailController::UnlockMode() {
  logic_.UnlockMode();
}
