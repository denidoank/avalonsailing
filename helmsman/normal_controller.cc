// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

//#include "common/boat.h"  // constants from simulation/boat.m

#include "helmsman/normal_controller.h"

#include <math.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include "common/delta_angle.h"
#include "common/normalize.h"
#include "common/now.h"
#include "common/limit_rate.h"
#include "common/polar_diagram.h"
#include "common/unknown.h"
#include "helmsman/apparent.h"
#include "helmsman/new_gamma_sail.h"
#include "helmsman/sampling_period.h"

extern int debug;

NormalController::NormalController(RudderController* rudder_controller,
                                   SailController* sail_controller)
   : rudder_controller_(rudder_controller),
     sail_controller_(sail_controller),
     skip_alpha_star_shaping_(false),
     alpha_star_rate_limit_(Deg2Rad(4) * kSamplingPeriod),  // 4 deg/s
     alpha_star_rate_limited_(0),
     alpha_star_restricted_(0),
     prev_alpha_star_restricted_(0),
     give_up_counter_(0),
     start_time_ms_(now_ms()),
     trap2_(999) {
  if (debug) fprintf(stderr, " NormalController::Entry time  %lld s\n",
                     start_time_ms_ / 1000 );
  CHECK_EQ(999, trap2_);
}

NormalController::~NormalController() {}

void NormalController::Entry(const ControllerInput& in,
                             const FilteredMeasurements& filtered) {
  // Make sure we get the right idea of the direction change when we come
  // from another state.
  prev_alpha_star_restricted_ = SymmetricRad(filtered.phi_z_boat);
  alpha_star_rate_limited_ = SymmetricRad(filtered.phi_z_boat);
  ref_.SetReferenceValues(prev_alpha_star_restricted_, in.drives.gamma_sail_rad);
  give_up_counter_ = 0;
  if (debug) {
    fprintf(stderr, " NormalController::Entry alpha star_limited: %lf\n",  prev_alpha_star_restricted_);
    start_time_ms_ = now_ms();
    fprintf(stderr, "Entry Time: %6.3lf %6.3lf s\n", (double)now_ms(), (double)start_time_ms_);
  }
}

void NormalController::Run(const ControllerInput& in,
                           const FilteredMeasurements& filtered,
                           ControllerOutput* out) {
  if (debug) {
    fprintf(stderr, "------------NormalController::Run----------\n");
    //fprintf(stderr, "Time ms: %lf  strt was %lf diff %lf \n", (double)now_ms(), (double)(start_time_ms_), (double)(now_ms()-start_time_ms_));
    //fprintf(stderr, "Time: %6.3lf Ref: %6.1lf ", NowSeconds(), Rad2Deg(in.alpha_star_rad));
    fprintf(stderr, "Actuals: True %6.1lf deg %6.1lf m/s ", Rad2Deg(filtered.alpha_true), filtered.mag_true);
    fprintf(stderr, "Actuals: Boat %6.1lf deg %6.1lf m/s ", Rad2Deg(filtered.phi_z_boat), filtered.mag_boat);
    fprintf(stderr, "Actuals: App  %6.1lf deg %6.1lf m/s\n", Rad2Deg(filtered.angle_app),  filtered.mag_app);
  }
  CHECK_EQ(999, trap2_);  // Triggers at incomplete compilation errors.
  double phi_star;
  double omega_star;
  double gamma_sail_star;
  ManeuverType maneuver = ReferenceValueSwitch(SymmetricRad(in.alpha_star_rad),
                       SymmetricRad(filtered.alpha_true), filtered.mag_true,
                       SymmetricRad(filtered.phi_z_boat), filtered.mag_boat,
                       SymmetricRad(filtered.angle_app),  filtered.mag_app,
                       in.drives.gamma_sail_rad,
                       &phi_star,
                       &omega_star,
                       &gamma_sail_star);
  if (maneuver == kTack) {
    out->status.tacks++;
    // if (debug) fprintf(stderr, "\nTack\n\n");
  } else if (maneuver == kJibe) {
    out->status.jibes++;
    // if (debug) fprintf(stderr, "\nJibe\n\n");
  }

  if (debug) fprintf(stderr, "IntRef: %6.2lf %6.2lf\n", Rad2Deg(phi_star), Rad2Deg(omega_star));

  double gamma_rudder_star;
  // The boat speed is an unreliable measurement value. The NormalController can work only
  // with postive boat speeds, but at the transition from the InitialController the very slowly
  // boat speed may be still negative. We simply clip the speed here. If the speed gets too low then
  // we'll leave the NormalController anyway (see GiveUp() ).
  double positive_speed = std::max(0.1, filtered.mag_boat);
  rudder_controller_->Control(phi_star,
                              omega_star,
                              SymmetricRad(filtered.phi_z_boat),
                              filtered.omega_boat,
                              positive_speed,
                              &gamma_rudder_star);
  if (isnan(gamma_rudder_star)) {
    fprintf(stderr, "gamma_R* isnan: phi_star %6.2lfdeg, omega_star %6.2lf, phi_z_boat %6.2lf, omega_boat %6.2lf, mag_boat %6.2lf\n",
            Rad2Deg(phi_star), omega_star, SymmetricRad(filtered.phi_z_boat), filtered.omega_boat, filtered.mag_boat);
  }

  out->drives_reference.gamma_rudder_star_left_rad  = gamma_rudder_star;
  out->drives_reference.gamma_rudder_star_right_rad = gamma_rudder_star;
  out->drives_reference.gamma_sail_star_rad = gamma_sail_star;
  if (debug) fprintf(stderr, "Controls: %6.2lf %6.2lf\n", Rad2Deg(gamma_rudder_star), Rad2Deg(gamma_sail_star));
}

void NormalController::Exit() {
  if (debug) fprintf(stderr, " NormalController::Exit\n");
}

bool NormalController::IsJump(double old_direction, double new_direction) {
  // All small direction changes are handled directly by the rudder control,
  // without reference value shaping or planning.
  // Limit for the magnitude of jumps in the reference values to differentiate
  // between continuous direction changes and maneuvers (tack or jibe).
  const double JibeZoneWidth = M_PI - JibeZoneRad();
  CHECK(JibeZoneWidth < TackZoneRad());
  return fabs(DeltaOldNewRad(old_direction, new_direction)) >
              1.8 * JibeZoneWidth;
}

bool NormalController::ShapeAlphaStar(double alpha_star,
                                      double alpha_true_wind,
                                      double* alpha_star_restricted) {
  // Rate limit alpha_star, rather slow now.
  //const double alpha_star_rate_limit_ = Deg2Rad(4) * kSamplingPeriod;  // degrees per second
  LimitRateWrapRad(alpha_star, alpha_star_rate_limit_, &alpha_star_rate_limited_);

  // Stay in sailable zone
  double new_alpha_star_restricted = BestSailableHeading(alpha_star_rate_limited_, alpha_true_wind);

  bool jump = IsJump(alpha_star_restricted_, new_alpha_star_restricted);
  alpha_star_restricted_ = new_alpha_star_restricted;
  *alpha_star_restricted = new_alpha_star_restricted;

  return jump;
}

ManeuverType NormalController::ReferenceValueSwitch(double alpha_star,
                                                    double alpha_true, double mag_true,
                                                    double phi_z_boat, double mag_boat,
                                                    double angle_app,  double mag_app,
                                                    double old_gamma_sail,
                                                    double* phi_z_star,
                                                    double* omega_z_star,
                                                    double* gamma_sail_star) {
  bool jump = false;
  double alpha_star_restricted = alpha_star;
  if (!skip_alpha_star_shaping_) {
    jump = ShapeAlphaStar(alpha_star, alpha_true, &alpha_star_restricted);
  }
  if (debug) fprintf(stderr, "* %6.2lf %6.2lf %6.2lf\n", alpha_star, alpha_star_rate_limited_, alpha_star_restricted);

  ManeuverType maneuver_type = kChange;
  if (!ref_.RunningPlan() && jump) {
    double new_gamma_sail;
    double delta_gamma_sail;
    maneuver_type = FindManeuverType(prev_alpha_star_restricted_,
                                     alpha_star_restricted,
                                     alpha_true);
    if (debug) fprintf(stderr, "Maneuver Type %d %lg %lg\n", maneuver_type, prev_alpha_star_restricted_, alpha_star_restricted);
    NewGammaSailWithOldGammaSail(alpha_true, mag_true,
                                 phi_z_boat, mag_boat,
                                 alpha_star_restricted,
                                 old_gamma_sail,
                                 maneuver_type,
                                 sail_controller_,
                                 &new_gamma_sail,
                                 &delta_gamma_sail);
    ref_.SetReferenceValues(prev_alpha_star_restricted_, old_gamma_sail);
    ref_.NewPlan(alpha_star_restricted,
                 delta_gamma_sail,
                 mag_boat);

    ref_.GetReferenceValues(phi_z_star, omega_z_star, gamma_sail_star);
  } else if (ref_.RunningPlan()) {
    // The plan includes a stabilization period of a second at the end
    // with constant reference values.
    ref_.GetReferenceValues(phi_z_star, omega_z_star, gamma_sail_star);
    // We don't accept new reference values here, so we also don't follow up
    // with the previous one.
  } else {
    // Normal case, minor changes of the desired heading.
    *phi_z_star = alpha_star_restricted;
    *omega_z_star = 0;
    // The apparent wind data are quickly filtered to suppress noise in
    // the sail drive reference value.
    *gamma_sail_star =
        sail_controller_->BestStabilizedGammaSail(angle_app, mag_app);
  }
  prev_alpha_star_restricted_ = alpha_star_restricted;
  return maneuver_type;
}

bool NormalController::TackingOrJibing() {
  return ref_.RunningPlan();
}

bool NormalController::GiveUp(const ControllerInput& in,
                              const FilteredMeasurements& filtered) {
  // Got stuck for more than a minute.
  if (filtered.mag_boat < 0.05)  // Check whether the speed info is reliable
    ++give_up_counter_;
  else
    give_up_counter_ = 0;
  return give_up_counter_ > 60.0 / kSamplingPeriod;
}

double NormalController::NowSeconds() {
  return (now_ms() - start_time_ms_) / 1000.0;
}

void NormalController::SkipAlphaStarShaping(bool skip) {
  skip_alpha_star_shaping_ = skip;
}
