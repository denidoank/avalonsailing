// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011


#include "sail_controller.h"

#include <math.h>
#include <stdio.h>

#include "lib/check.h"
#include "lib/convert.h"
#include "lib/sign.h"
#include "lib/normalize.h"

#include "sail_controller_const.h"
#include "sampling_period.h"  // kSamplingPeriod

extern int debug;

SailModeLogic::SailModeLogic()
    : mode_(WING),
      delay_counter_(0)
  {}

// apparent wind angle is not negative.
SailMode SailModeLogic::BestMode(double apparent_absolute, double wind_strength_m_s) {
  mode_ = apparent_absolute < kSwitchpoint && wind_strength_m_s < kSpinakkerLimit ? SPINNAKER : WING;
  return mode_;
}

SailMode SailModeLogic::BestStabilizedMode(double apparent_absolute, double wind_strength_m_s) {
  const int delay = static_cast<int>(kSwitchBackDelay / kSamplingPeriod + 0.5);
  if (mode_ == WING_LOCKED) {
    return WING_LOCKED;
  }
  if (wind_strength_m_s > kSpinakkerLimit) {
    return WING;
  }
  if (mode_ == WING) {
    if (apparent_absolute <= kSwitchpoint - 2 * kHalfHysteresis ||
        (apparent_absolute < kSwitchpoint - kHalfHysteresis &&
         ++delay_counter_ > delay)) {
      mode_ = SPINNAKER;
      delay_counter_ = 0;
      if (debug) fprintf(stderr, "SailModeLogic::BestMode: Switched to spinnaker.\n");
    }
  } else {  // SPINNAKER
    if (apparent_absolute > kSwitchpoint + 2 * kHalfHysteresis ||
        (apparent_absolute > kSwitchpoint + kHalfHysteresis &&
         ++delay_counter_ > delay)) {
      mode_ = WING;
      delay_counter_ = 0;
      if (debug) fprintf(stderr, "SailModeLogic::BestMode: Switched to wing.\n");
    }
  }
  return mode_;
}

void SailModeLogic::UnlockMode() {
  mode_ = WING;
}

void SailModeLogic::Reset() {
  mode_ = WING;
  delay_counter_ = 0;
}

namespace {
const char kWing[] = "WING";
const char kSpinnaker[] = "SPINNAKER";
const char kWingLocked[] = "WING_LOCKED";
}  // namespace

const char* SailModeLogic::ModeString() {
  if (debug)
    fprintf(stderr, "delay_counter_: %d\n", delay_counter_);
  if (mode_ == WING) return kWing;
  if (mode_ == SPINNAKER) return kSpinnaker;
  if (mode_ == WING_LOCKED) return kWingLocked;
  CHECK(0);
  return NULL;
}


// The optimal angle of attack for the trimmed sail, subject to
// optimization, 20 - 40 degrees. One must not forget that this
// is the boom angle. The sail towards the mast top is twisted
// and has a lesser angle of attack.
static const double kAOADefaultRad = M_PI / 5;  // Deg2Rad(36.0);
SailController::SailController()
    : optimal_angle_of_attack_rad_(kAOADefaultRad),
      sign_(1) {
  CHECK_LT(0.1, optimal_angle_of_attack_rad_);
}

void SailController::SetOptimalAngleOfAttack(double optimal_angle_of_attack_rad) {
  optimal_angle_of_attack_rad_ = optimal_angle_of_attack_rad;
}

double SailController::GetOptimalAngleOfAttack() {
  CHECK_LT(0.1, optimal_angle_of_attack_rad_);  // paranoid
  return optimal_angle_of_attack_rad_;
}

double SailController::BestGammaSail(double alpha_wind_rad, double mag_wind) {
  return GammaSailInternal(alpha_wind_rad, mag_wind, false);
}

double SailController::BestStabilizedGammaSail(double alpha_wind_rad, double mag_wind) {
  // fprintf(stderr, "mode %s wind %6.2lf mag %6.2lf sign %d\n", logic_.ModeString(), alpha_wind_rad, mag_wind, sign_);
  return GammaSailInternal(alpha_wind_rad, mag_wind, true);
}

double SailController::AngleOfAttack(double mag_wind) {
  // The sail forces are proportional to the square of the wind speed.
  if (mag_wind < kAngleReductionLimit) {
    return optimal_angle_of_attack_rad_;
  } else {
    return optimal_angle_of_attack_rad_ * kAngleReductionLimit * kAngleReductionLimit /
        (mag_wind * mag_wind);
  }
}

double SailController::GammaSailInternal(double alpha_wind_rad,
                                         double mag_wind,
                                         bool stabilized) {
  CHECK_IN_INTERVAL(-10, alpha_wind_rad, 10);
  CHECK_LT(0.1, optimal_angle_of_attack_rad_);

  alpha_wind_rad = SymmetricRad(alpha_wind_rad);
  HandleSign(&alpha_wind_rad, &sign_);
  CHECK_IN_INTERVAL(0, alpha_wind_rad, M_PI);

  // other lower limit, to avoid unnecessary sail motor activity at low winds?
  if (mag_wind == 0)
    return 0;

  SailMode mode = stabilized ?
      logic_.BestStabilizedMode(alpha_wind_rad, mag_wind) :
      logic_.BestMode(alpha_wind_rad, mag_wind);

  double gamma_sail_rad = (mode == WING) ?
      (alpha_wind_rad - M_PI + AngleOfAttack(mag_wind)) :
      (0.5 * alpha_wind_rad - kDragMax);      // SPINNAKER mode, broad reach

  return SymmetricRad(sign_ * gamma_sail_rad);
}

double SailController::BestGammaSailForReverseMotion(double alpha_wind_rad,
                                                     double mag_wind) {
  // Avoid unnecessary sail motor activity at low wind.
  if (mag_wind < 0.5)
    return M_PI / 2;

  assert(alpha_wind_rad < 10);
  assert(-alpha_wind_rad > -10);
  alpha_wind_rad = SymmetricRad(alpha_wind_rad);
  int sign = 1;
  if (alpha_wind_rad < 0) {
    sign = -1;
    alpha_wind_rad = -alpha_wind_rad;
  }
  CHECK_LE(alpha_wind_rad, M_PI);  // in [0, pi]

  double gamma_sail_rad = alpha_wind_rad < (M_PI - kSwitchpoint) || mag_wind > kSpinakkerLimit ?
      (M_PI - alpha_wind_rad + AngleOfAttack(mag_wind)) :
      M_PI / 2;      // reversed SPINNAKER mode, broad reach

  return SymmetricRad(-sign * gamma_sail_rad);
}

void SailController::UnlockMode() {
  logic_.UnlockMode();
}

void SailController::HandleSign(double* alpha_wind_rad, int* sign ) {
  *sign = SignNotZero(*alpha_wind_rad);
  *alpha_wind_rad *= *sign;
}

void SailController::Reset() {
  logic_.Reset();
  optimal_angle_of_attack_rad_ = kAOADefaultRad;
  sign_ = 1;
}
