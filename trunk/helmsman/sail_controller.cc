// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011


#include "helmsman/sail_controller.h"

#include <algorithm>  // max

#include <math.h>
#include <stdio.h>

#include "common/apparent.h"
#include "common/check.h"
#include "common/convert.h"
#include "common/delta_angle.h"
#include "common/sign.h"
#include "common/normalize.h"
#include "common/Polar_diagram.h"

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
    mode_ = WING;
    delay_counter_ = 0;
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
  if (delay_counter_ > 1000)
    delay_counter_ = 1000;  // Avoid wrap around to negative counts.
  if (debug) fprintf(stderr, "SailModeLogic::BestMode: delay_counter: %d, delay %d\n",
                     delay_counter_, delay);
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
      app_sign_(1) {
  CHECK_LT(0.1, optimal_angle_of_attack_rad_);
}

void SailController::SetOptimalAngleOfAttack(double optimal_angle_of_attack_rad) {
  optimal_angle_of_attack_rad_ = optimal_angle_of_attack_rad;
}

double SailController::GetOptimalAngleOfAttack() {
  CHECK_LT(0.1, optimal_angle_of_attack_rad_);  // paranoid
  return optimal_angle_of_attack_rad_;
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

Angle SailController::ReverseGammaSailFromApparent(const Polar& apparent_wind) {
  // Avoid unnecessary sail motor activity at low wind.
  if (apparent_wind.Mag() < 0.5)
    return deg(app_sign_ * -120);

  Polar zero(0, 0);
  Polar reversed_apparent_wind = zero - apparent_wind;
  app_sign_ *= -1;
  Angle gamma = StableGammaSailFromApparent(reversed_apparent_wind);
  app_sign_ *= -1;
  return gamma.opposite();
}

void SailController::Reset() {
  logic_.Reset();
  optimal_angle_of_attack_rad_ = kAOADefaultRad;
}

// No automatic switch of the tack (apparent wind sign).
void SailController::SetAppSign(int sign) {
  app_sign_ = sign;
}

Angle SailController::StableGammaSail(const Polar& true_wind,     // double alpha_true, double mag_true,
                                      const Polar& apparent_wind, // double alpha_app, double mag_app,
                                      Angle phi_z) {
  Angle alpha_true = true_wind.Arg();
  double mag_true = true_wind.Mag();

  // We reconstruct the apparent wind from the slowly filtered true wind
  // data and the stable phi_z value. Thus the apparent wind willnot cause
  // oscillations.
  double boat_speed_m_s;
  bool dummy1, dummy2;
  ReadPolarDiagram((alpha_true - phi_z).opposite().deg(),
                   mag_true,
                   &dummy1,
                   &dummy2,
                   &boat_speed_m_s);
  Polar boat(phi_z, boat_speed_m_s);
  Polar apparent_reconstructed(0, 0);
  ApparentPolar(true_wind,
                boat,
                phi_z,
                &apparent_reconstructed);

  Angle a = phi_z - alpha_true;
  a = -apparent_reconstructed.Arg();
  return StableGammaSailFromApparent(apparent_reconstructed);
}

// Use this if the true wind is unavailable.
Angle SailController::StableGammaSailFromApparent(const Polar& apparent_wind) { // double alpha_app, double mag_app,
  double mag_app = apparent_wind.Mag();

  /* points of sail.
     alpha = apparent wind, sometimes approximated by (phi_z - alpha_true_wind).


                                      |  true wind
                                      |
                                      |
                                      V
 (close hauled starboard) a=-135deg       a=+135deg (close hauled, portside tack)

     (broad reach starb.) a=-90 deg       a=+90deg (broad reach, portside tack)

                                     a=0 (running)
  */

  Angle a = apparent_wind.Arg();

  // The sign of gamma_sail is opposite to app_sign_.
  // The sign of the apparent wind angle is identical to app_sign_.
  // 2 exceptions:
  // * When running (wind blowing into the same direction as we go)
  //   the wind may change a little bit, but we keep the sail on the
  //   "wrong" side. No action.
  // * If a tack failed, or a wave pushed us violently. We are fexible and
  //   try to catch the wind on the other side. This is handled by the
  //   sector logic polar_diagram.cc::SailableHeading()
  a = a * app_sign_;
  if (a > deg(179))
    a = deg(179);
  Angle gamma_sail;
  // The optimal sail mode is strictly speaking dependant on the apparent wind.
  // The differences are small, so we use the more stable true wind angle.
  if (SPINNAKER == logic_.BestStabilizedMode(a.rad(), mag_app)) {
    gamma_sail = rad(kDragMax) - a / 2;
    if (debug) fprintf(stderr, " spi: %lf\n", gamma_sail.rad());
  } else {
    gamma_sail = (-a - rad(AngleOfAttack(mag_app))).opposite();
    if (debug) fprintf(stderr, " wng: %lf rad, aoa %lf rad\n",
                       gamma_sail.rad(), AngleOfAttack(mag_app));
    // When sailing close hauled (hard to the wind) we observed sail angle oscillations
    // leading to the sail swinging over the boat symmetry axis. This will be
    // suppressed. For forward
    const int kMinimumSailAngle = 13;
    if (gamma_sail < deg(13))
      gamma_sail = deg(13);
    if (a > deg(179 - kMinimumSailAngle))
      gamma_sail = 0;
  }

  // The sail front end alway points into the wind.
  return app_sign_ == 1 ? -gamma_sail : gamma_sail;
}
