// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/new_gamma_sail.h"

#include "common/delta_angle.h"
#include "common/normalize.h"
#include "common/polar.h"
#include "common/sign.h"
#include "helmsman/apparent.h"

#include <math.h>
#include <stdio.h>

// TODO(grundmann): Clean this up, make it nicer with Polar.
void NewGammaSail(double alpha_true, double mag_true,
                  double alpha_boat, double mag_boat,
                  double new_alpha_boat,
                  ManeuverType maneuver_type,
                  SailController* sail_controller,
                  double* new_gamma_sail,
                  double* delta_gamma_sail) {
  new_alpha_boat = SymmetricRad(new_alpha_boat);               
  // We assume that the boat speed is constant during tack (or a little less)
  double new_mag_boat = 1.0 * mag_boat;
  double alpha_app;
  double mag_app;
  Apparent(alpha_true, mag_true, alpha_boat, mag_boat, alpha_boat, &alpha_app, &mag_app);
  // apparent.Print("\nold_apparent");
  double old_gamma_sail = sail_controller->BestGammaSail(alpha_app, mag_app);
  // fprintf(stderr, "old gamma: %6.2g deg\n", Rad2Deg(old_gamma_sail));   
  double new_alpha_app;
  double new_mag_app;
  Apparent(alpha_true, mag_true, new_alpha_boat, new_mag_boat, new_alpha_boat, &new_alpha_app, &new_mag_app);

  fprintf(stderr, "alpha_app %6.2f deg , new_alpha_app %6.2f\n", Rad2Deg(alpha_app), Rad2Deg(new_alpha_app));
  double est = alpha_app - (new_alpha_boat - alpha_boat);
  fprintf(stderr, "est %6.2f deg , delta %6.2f\n", Rad2Deg(est), Rad2Deg(est - new_alpha_app));


  *new_gamma_sail = sail_controller->
      BestGammaSail(new_alpha_app, new_mag_app);
  // fprintf(stderr, "new gamma: %6.2f deg\n", Rad2Deg(*new_gamma_sail));   
  double delta = *new_gamma_sail - old_gamma_sail;   // TODO check usage of  DeltaOldNewRad here!
  // fprintf(stderr, "old gamma: %6.2f deg\n", Rad2Deg(old_gamma_sail));   
  // fprintf(stderr, "delta: %6.2f deg %g\n", Rad2Deg(delta), Sign(delta));   
  
  if (maneuver_type == kJibe)
    *delta_gamma_sail = delta - 2 * M_PI * Sign(delta);
  else
    *delta_gamma_sail = delta; 
}

void NewGammaSailWithOldGammaSail(
                  double alpha_true, double mag_true,
                  double alpha_boat, double mag_boat,
                  double new_alpha_boat,
                  double old_gamma_sail,
                  ManeuverType maneuver_type,
                  SailController* sail_controller,
                  double* new_gamma_sail,
                  double* delta_gamma_sail) {
  new_alpha_boat = SymmetricRad(new_alpha_boat);
  // We assume that the boat speed is constant during tack (or a little less)
  double new_mag_boat = 1.0 * mag_boat;
  double alpha_app;
  double mag_app;
  Apparent(alpha_true, mag_true, alpha_boat, mag_boat, alpha_boat, &alpha_app, &mag_app);
  // apparent.Print("\nold_apparent");
  // fprintf(stderr, "old gamma: %6.2lg deg\n", Rad2Deg(old_gamma_sail));
  double new_alpha_app;
  double new_mag_app;
  Apparent(alpha_true, mag_true, new_alpha_boat, new_mag_boat, new_alpha_boat, &new_alpha_app, &new_mag_app);
  fprintf(stderr, "exact_app_new %6.2lf ", Rad2Deg(new_alpha_app));

  // fprintf(stderr, "alpha_app %6.2lf deg , new_alpha_app %6.2lf", Rad2Deg(alpha_app), Rad2Deg(new_alpha_app));

  *new_gamma_sail = sail_controller->
      BestGammaSail(new_alpha_app, new_mag_app);
  // fprintf(stderr, "new gamma: %6.2lf deg\n", Rad2Deg(*new_gamma_sail));
  double delta = *new_gamma_sail - old_gamma_sail;
  // fprintf(stderr, "old gamma: %6.2lf deg\n", Rad2Deg(old_gamma_sail));
  // fprintf(stderr, "delta: %6.2lf deg %lg\n", Rad2Deg(delta), Sign(delta));

  if (maneuver_type == kJibe)
    *delta_gamma_sail = delta - 2 * M_PI * Sign(delta);
  else
    *delta_gamma_sail = delta;
}

namespace {

// Motion effect compensation for the new angle.
double MotionEffect1(double alpha_raw) {
  fprintf(stderr, "alpha_raw1: %lf ", Rad2Deg(alpha_raw));
  CHECK_LE(0, alpha_raw);
  CHECK_LE(alpha_raw, M_PI);
  // See MotionEffecyStudy.ods
  // =s_angle * SIN(PI()*POWER(K2/PI(), 0.75))
  // arc sine of the typical ratio of speed to wind from the Polar diagram.
  const double s_angle = Deg2Rad(20.7);
  // This is not cheap, but we do it once for every maneuver only.
  return s_angle * sin(M_PI * pow(alpha_raw / M_PI, 0.75));
}

// Motion effect for the old apparent wind angle.
double MotionEffect2(double alpha_raw) {
  fprintf(stderr, "alpha_raw2: %lf ", Rad2Deg(alpha_raw));
  CHECK_LE(0, alpha_raw);
  CHECK_LE(alpha_raw, M_PI);
  // See reverse.ods
  const double s_angle = Deg2Rad(21.7);
  return -s_angle * sin(alpha_raw);
}

double MotionEffect(double alpha_raw, bool old_angle) {
  alpha_raw = SymmetricRad(alpha_raw);
  int sign_alpha = 1;
  if (alpha_raw < 0) {
    alpha_raw =  -alpha_raw;
    sign_alpha = -1;
  }
  if (old_angle)
    return sign_alpha * MotionEffect2(alpha_raw);
  else
    return sign_alpha * MotionEffect1(alpha_raw);
}

double MotionEffectCombined(double alpha_old, double alpha_raw) {
  double effect_old = MotionEffect(SymmetricRad(alpha_old), true);
  // fprintf(stderr, "effect old: %6.2lf ", Rad2Deg(effect_old));

  double effect = MotionEffect(SymmetricRad(alpha_raw), false);
  // fprintf(stderr, "effect new: %6.2lf ", Rad2Deg(effect));

  return effect + effect_old;
}

}  // namespace

void NextGammaSailWithOldGammaSail(
                  double old_alpha_app, double old_mag_app,
                  double old_alpha_boat,
                  double new_alpha_boat,
                  double old_gamma_sail,
                  ManeuverType maneuver_type,
                  SailController* sail_controller,
                  double* new_gamma_sail,
                  double* delta_gamma_sail) {
  old_alpha_app = SymmetricRad(old_alpha_app);
  old_alpha_boat = SymmetricRad(old_alpha_boat);
  new_alpha_boat = SymmetricRad(new_alpha_boat);
  old_gamma_sail = SymmetricRad(old_gamma_sail);

  // fprintf(stderr, "old_alpha_app %6.2f deg", Rad2Deg(old_alpha_app));
  // apparent winds
  double new_alpha_app_raw = SymmetricRad(old_alpha_app - (new_alpha_boat - old_alpha_boat));
  double new_alpha_app = SymmetricRad(new_alpha_app_raw +
                                      MotionEffectCombined(old_alpha_app,
                                                           new_alpha_app_raw));
  fprintf(stderr, "(new app raw/corr %6.2lf/%6.2lf)",
          Rad2Deg(new_alpha_app_raw), Rad2Deg(new_alpha_app));

  *new_gamma_sail = sail_controller->
      BestGammaSail(new_alpha_app, old_mag_app);
  // fprintf(stderr, "new gamma: %6.2f deg\n", Rad2Deg(*new_gamma_sail));
  double delta = *new_gamma_sail - old_gamma_sail;
  // fprintf(stderr, "old gamma: %6.2f deg\n", Rad2Deg(old_gamma_sail));
  // fprintf(stderr, "delta: %6.2f deg %g\n", Rad2Deg(delta), Sign(delta));

  if (maneuver_type == kJibe)
    *delta_gamma_sail = delta - 2 * M_PI * Sign(delta);
  else
    *delta_gamma_sail = delta;
}
