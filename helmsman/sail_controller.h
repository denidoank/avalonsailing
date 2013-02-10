// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_SAIL_CONTROLLER_H
#define HELMSMAN_SAIL_CONTROLLER_H

#include "common/angle.h"
#include "common/polar.h"

enum SailMode {
  WING,        // for sailing at the wind
  SPINNAKER    // optimal for broad reach
};

class SailModeLogic {
 public:
  SailModeLogic();
  // apparent is supposed to be the absolute value, but can go below zero (hysteresis effect).
  // Side effect: sets mode_
  SailMode BestMode(double alpha_apparent_wind_rad, double wind_strength_m_s);
  SailMode BestStabilizedMode(double alpha_apparent_wind_rad, double wind_strength_m_s);
  void UnlockMode();
  void Reset();
  const char* ModeString();
 private:
  SailMode mode_;  // need state for hysteresis
  int delay_counter_;
};

class SailController {
 public:
  SailController();

  // Set Tack, i.e. which side the wind is coming from.
  // Equal to the sign of the apparent wind
  // -1: from starboard (right) to portside,
  // +1: from portside (left) to starboard
  void SetAppSign(int sign);

  Angle StableGammaSail(const Polar& true_wind,
                        const Polar& apparent_wind,
                        Angle phi_z);

  // Use this if the true wind is unavailable.
  Angle StableGammaSailFromApparent(const Polar& apparent_wind);

  // For the initial controller we need a reverse gear of the sail
  // controller which works for apparent angles in (-180, -85) and
  // (85, 179) degrees resp. . Looking backwards this means (-95, 95 degrees)
  // for the apparent angle.
  // The spinakker sail mode must not be used for strong winds (risk of overloading
  // especially because the stern might dig into the waves.) So we use a wing mode
  // with an angle of attack of 50 degrees instead which produces smaller forces.
  // Needs a prior call of SetAlphaSign().
  Angle ReverseGammaSailFromApparent(const Polar& apparent_wind);

  void SetOptimalAngleOfAttack(double optimal_angle_of_attack_rad);
  double GetOptimalAngleOfAttack();
  void Reset();

 private:
  // Optimal angle of attack, reduced at high wind strength.
  double AngleOfAttack(double mag_wind);

  double optimal_angle_of_attack_rad_;
  SailModeLogic logic_;
  int app_sign_;  // +1 when sailing on portside tack (apparent wind from left to right).
};

#endif  // HELMSMAN_SAIL_CONTROLLER_H
