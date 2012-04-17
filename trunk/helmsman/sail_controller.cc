// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011


#include "helmsman/sail_controller.h"

#include <math.h>
#include <stdio.h>

#include "common/check.h"
#include "common/convert.h"
#include "common/sign.h"
#include "common/normalize.h"

#include "helmsman/sail_controller_const.h"
#include "helmsman/sampling_period.h"  // kSamplingPeriod

extern int debug;

SailModeLogic::SailModeLogic()
    : mode_(WING),
      delay_counter_(0)
  {}

SailMode SailModeLogic::BestMode(double apparent, double wind_strength_m_s) const {
  CHECK_GE(apparent, 0);
  return apparent < kSwitchpoint && wind_strength_m_s < kSpinakkerLimit ? SPINNAKER : WING;
}

SailMode SailModeLogic::BestStabilizedMode(double apparent, double wind_strength_m_s) {
  const int delay = static_cast<int>(kSwitchBackDelay / kSamplingPeriod + 0.5);
  if (mode_ == WING_LOCKED) {
    return WING_LOCKED;
  }
  if (wind_strength_m_s > kSpinakkerLimit)
    return WING;
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
      if (debug) fprintf(stderr, "SailModeLogic::BestMode: Switched to wing.\n");
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
// optimization, 10 - 25 degrees.
static const double kAOADefaultRad = M_PI / 9;  // Deg2Rad(20.0);

SailController::SailController()
    : optimal_angle_of_attack_rad_(kAOADefaultRad),
      sign_(1),
      debug_(false) {
  if (debug_) {
    fprintf(stderr, "SC: Ctor, optimal aoa: %lg deg\n",
          Rad2Deg(optimal_angle_of_attack_rad_));
    fprintf(stderr, "SC: Ctor, constant: %lg deg\n",
          Rad2Deg(kAOADefaultRad));
  }
  CHECK_LT(0.1, optimal_angle_of_attack_rad_);
}

void SailController::SetOptimalAngleOfAttack(double optimal_angle_of_attack_rad) {
  optimal_angle_of_attack_rad_ = optimal_angle_of_attack_rad;
  if (debug_) fprintf(stderr, "SC: SetOptimalAngleOfAttack, optimal aoa: %lg deg\n",
          Rad2Deg(optimal_angle_of_attack_rad_));

}

double SailController::GetOptimalAngleOfAttack() {
  CHECK_LT(0.1, optimal_angle_of_attack_rad_);
  return optimal_angle_of_attack_rad_;
}

double SailController::BestGammaSail(double alpha_wind_rad, double mag_wind) {
  return GammaSailInternal(alpha_wind_rad, mag_wind, false);
}

double SailController::BestStabilizedGammaSail(double alpha_wind_rad, double mag_wind) {
  double out = GammaSailInternal(alpha_wind_rad, mag_wind, true);
  if (debug_ || 1) {
    fprintf(stderr, "SC: BestStabilizedGammaSail, %s\nalpha %lg deg, mag %lg\n", logic_.ModeString(),
            Rad2Deg(alpha_wind_rad), mag_wind);
    fprintf(stderr, "SC: BestStabilizedGammaSail, optimal aoa: %lg deg, out %lg\n",
            Rad2Deg(optimal_angle_of_attack_rad_), Rad2Deg(out));
  }

  return out;
}

double SailController::AngleOfAttack(double mag_wind) {
  // The sail forces are proportional to the square of the wind speed.
  if (mag_wind < kAngleReductionLimit) {
    if (debug_) fprintf(stderr, "SC: AngleOfAttack, optimal aoa: %lg deg\n",
            Rad2Deg(optimal_angle_of_attack_rad_));

    return optimal_angle_of_attack_rad_;
  } else {
    if (debug_) {
      fprintf(stderr, "SC: AngleOfAttack, AngleReductionlimit reached with %lg m/s\n", mag_wind);
      fprintf(stderr, "SC: AngleOfAttack,aoa: %lg, optaoa: %lg\n  ", optimal_angle_of_attack_rad_ * kAngleReductionLimit * kAngleReductionLimit /
              (mag_wind * mag_wind), optimal_angle_of_attack_rad_);
    }
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
  alpha_wind_rad = HandleSign(alpha_wind_rad, stabilized);
  CHECK_LE(alpha_wind_rad, M_PI);  // in [0, pi]

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
  assert(alpha_wind_rad < 10);
  assert(-alpha_wind_rad > -10);
  alpha_wind_rad = SymmetricRad(alpha_wind_rad);
  int sign = 1;
  if (alpha_wind_rad < 0) {
    sign = -1;
    alpha_wind_rad = -alpha_wind_rad;
  }
  CHECK_LE(alpha_wind_rad, M_PI);  // in [0, pi]

  // other lower limit, to avoid unnecessary sail motor activity at low winds?
  if (mag_wind == 0)
    return M_PI / 2;

  double gamma_sail_rad = alpha_wind_rad < (M_PI - kSwitchpoint) || mag_wind > kSpinakkerLimit ?
      (M_PI - alpha_wind_rad + AngleOfAttack(mag_wind)) :
      M_PI / 2;      // reversed SPINNAKER mode, broad reach

  return SymmetricRad(-sign * gamma_sail_rad);
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
  optimal_angle_of_attack_rad_ = kAOADefaultRad;
  if (debug_) fprintf(stderr, "SC: Reset, optimal aoa: %lg deg\n",
          Rad2Deg(optimal_angle_of_attack_rad_));
  sign_ = 1;
}

