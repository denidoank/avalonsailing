// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011


#include "helmsman/sail_controller.h"

#include <algorithm>  // max

#include <math.h>
#include <stdio.h>

#include "common/check.h"
#include "common/convert.h"
#include "common/delta_angle.h"
#include "common/sign.h"
#include "common/normalize.h"

#include "helmsman/sail_controller_const.h"
#include "helmsman/sampling_period.h"  // kSamplingPeriod

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
  const int delay = static_cast<int>(kSpinakkerSwitchDelay / kSamplingPeriod + 0.5);
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


void SailModeLogic::Reset() {
  mode_ = WING;
  delay_counter_ = 0;
}

namespace {
const char kWing[] = "WING";
const char kSpinnaker[] = "SPINNAKER";
}  // namespace

const char* SailModeLogic::ModeString() {
  if (debug)
    fprintf(stderr, "delay_counter_: %d\n", delay_counter_);
  if (mode_ == WING) return kWing;
  if (mode_ == SPINNAKER) return kSpinnaker;
  CHECK(0);
  return NULL;
}


// The optimal angle of attack for the trimmed sail, subject to
// optimization, 20 - 40 degrees. One must not forget that this
// is the boom angle. The sail towards the mast top is twisted
// and has a lesser angle of attack.
static const double kAOADefaultRad = M_PI / 4;  // Deg2Rad(45);
SailController::SailController()
    : optimal_angle_of_attack_rad_(kAOADefaultRad),
      sign_(1),
      alpha_sign_(1) {
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

void SailController::HandleSign(double* alpha_wind_rad, int* sign ) {
  *sign = SignNotZero(*alpha_wind_rad);
  *alpha_wind_rad *= *sign;
}

void SailController::Reset() {
  logic_.Reset();
  optimal_angle_of_attack_rad_ = kAOADefaultRad;
  sign_ = 1;
}

// No automatic switch of the tack (sail position, Luv side).
void SailController::SetAlphaSign(int sign) {
  alpha_sign_ = sign;
}

double SailController::StableGammaSail(double alpha_true, double mag_true,
                                       double alpha_app, double mag_app,
                                       double phi_z,
                                       double* phi_z_offset) {
  /* points of sail
     alpha = phi_z - alpha_true_wind


                                      |  true wind
                                      |
                                      |
                                      V
 (close hauled starboard) a=+135deg       a=-135deg (close hauled, portside tack)

     (broad reach starb.) a=+90 deg       a=-90deg (broad reach, portside tack)

                                     a=0 (running)
  */

  // which tack are we on, with a lot of robustness for a stable sail position when running.

  double a = DeltaOldNewRad(alpha_true, phi_z);
  if (debug) fprintf(stderr, "sca: %lf\n", a);


  if (alpha_sign_ < 0 && a > kTackHysteresis) {
    //CHECK_EQ(1, alpha_sign_); // TODO convert into warning after tests.
  }

  if (alpha_sign_ > 0 && a < -kTackHysteresis) {
    //CHECK_EQ(-1, alpha_sign_); // TODO convert into warning after tests.
  }

  // The sign of gamma_sail is always identical with alpha_sign_!

  // Push the boat (phi_z) out of the wind if the quickly filtered apparent
  // wind is too adverse.
  const double kCloseHauledLimit = Deg2Rad(140);
  if (alpha_sign_ == -1 && alpha_app > kCloseHauledLimit) {
    if (debug) fprintf(stderr, "StableSail:: Too close, fall off right\n");
    *phi_z_offset = alpha_app - kCloseHauledLimit;
  }
  if (alpha_sign_ == 1 && alpha_app < -kCloseHauledLimit) {
    if (debug) fprintf(stderr, "StableSail:: Too close, fall off left\n");
    *phi_z_offset = alpha_app + kCloseHauledLimit;
  }
  a -= *phi_z_offset;
  if (debug) fprintf(stderr, "new phi_z_offset: %lf", *phi_z_offset);

  // Sailing physics is symmetric
  a = fabs(a);

  double gamma_sail_rad;
  // The optimal sail mode is strictly speaking dependant on the apparent wind.
  // The differences are small, so we use the more stable true wind angle.
  if (SPINNAKER == logic_.BestStabilizedMode(a, std::max(mag_app, mag_true))) {
    gamma_sail_rad = kDragMax - a / 2;
    if (debug) fprintf(stderr, "spi: %lf", gamma_sail_rad);
  } else {
    gamma_sail_rad = M_PI - a - AngleOfAttack(mag_true);
    if (debug) fprintf(stderr, "wng: %lf", gamma_sail_rad);
    // When sailing close hauled (hard to the wind) we observed sail angle oscillations
    // leading to the sail swinging over the boat symmetry axis. This will be
    // suppressed.
    const double kCloseHauledLimit = Deg2Rad(6);
    if (gamma_sail_rad < kCloseHauledLimit)
      gamma_sail_rad = kCloseHauledLimit;
  }
  if (debug) {
      fprintf(stderr, "StableSail:: sign %d %lf\n",
                       alpha_sign_, gamma_sail_rad);
      fprintf(stderr, "StableSail:: out: %lf\n",
                     SymmetricRad(alpha_sign_ * gamma_sail_rad));
  }

  return SymmetricRad(alpha_sign_ * gamma_sail_rad);

}




