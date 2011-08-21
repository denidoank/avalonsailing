// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

//#include "common/boat.h"  // constants from simulation/boat.m

#include "helmsman/normal_controller.h"

#include <math.h>
#include "common/delta_angle.h"
#include "common/normalize.h"
#include "common/polar_diagram.h"
#include "common/limit_rate.h"
#include "common/unknown.h"
#include "helmsman/apparent.h"
#include "helmsman/new_gamma_sail.h"
#include "helmsman/sampling_period.h"

extern int debug;

// TODO(grundmann): Tack Overshoot.

NormalController::NormalController(RudderController* rudder_controller,
                                   SailController* sail_controller)
   : rudder_controller_(rudder_controller),
     sail_controller_(sail_controller),
     prev_alpha_star_limited_(0),
     alpha_star_smooth_(0),
     give_up_counter_(0) {}

NormalController::~NormalController() {}

void NormalController::Entry(const ControllerInput& in,
                             const FilteredMeasurements& filtered){
  // Make sure we get the right idea of the direction change when we come
  // from another state.
  prev_alpha_star_limited_ = SymmetricRad(filtered.phi_z_boat);
  alpha_star_smooth_ = SymmetricRad(filtered.phi_z_boat);
  ref_.SetReferenceValues(prev_alpha_star_limited_, in.drives.gamma_sail_rad);
  give_up_counter_ = 0;
  if (debug) fprintf(stderr, " NormalController::Entry alpha star_limited: %lf\n",  prev_alpha_star_limited_);

}

void NormalController::Run(const ControllerInput& in,
                           const FilteredMeasurements& filtered,
                           ControllerOutput* out) {
  if (debug) {
    fprintf(stderr, "------------NormalController::Run----------\n");
    fprintf(stderr, "Ref: %6.4f", Rad2Deg(in.alpha_star_rad));
    fprintf(stderr, "Actuals: True %6.4f deg %6.4f m/s", Rad2Deg(filtered.alpha_true), filtered.mag_true);
    fprintf(stderr, "Actuals: Boat %6.4f deg %6.4f m/s", Rad2Deg(filtered.phi_z_boat), filtered.mag_boat);
    fprintf(stderr, "Actuals: App  %6.4f deg %6.4f m/s\n", Rad2Deg(filtered.angle_app),  filtered.mag_app);
  }

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
  if (maneuver == kTack)
    out->status.tacks++;
  else if (maneuver == kJibe)
    out->status.jibes++;

  if (debug) fprintf(stderr, "IntRef: %6.4f %6.4f %6.4f\n", Rad2Deg(phi_star), Rad2Deg(omega_star), Rad2Deg(gamma_sail_star));

  double gamma_rudder_star;
  rudder_controller_->Control(phi_star,
                              omega_star,
                              SymmetricRad(filtered.phi_z_boat),
                              filtered.omega_boat,
                              filtered.mag_boat,
                              &gamma_rudder_star);

  out->drives_reference.gamma_rudder_star_left_rad  = gamma_rudder_star;
  out->drives_reference.gamma_rudder_star_right_rad = gamma_rudder_star;
  out->drives_reference.gamma_sail_star_rad = gamma_sail_star;
  if (debug) fprintf(stderr, "Controls: %6.4f %6.4f\n", Rad2Deg(gamma_rudder_star), Rad2Deg(gamma_sail_star));
}

void NormalController::Exit() {
  if (debug) fprintf(stderr, " NormalController::Exit\n");
}

ManeuverType NormalController::ReferenceValueSwitch(double alpha_star,
                                            double alpha_true, double mag_true,
                                            double phi_z_boat, double mag_boat,
                                            double angle_app,  double mag_app,
                                            double old_gamma_sail,
                                            double* phi_z_star,
                                            double* omega_z_star,
                                            double* gamma_sail_star) {
  ManeuverType maneuver_type = kChange;
  // Rate limit alpha_star, rather slow now.
  const double rate_limit = Deg2Rad(4) * kSamplingPeriod;  // degrees per second
  LimitRateWrapRad(alpha_star, rate_limit, &alpha_star_smooth_);

  // Stay in sailable zone
  double alpha_star_limited = BestSailableHeading(alpha_star_smooth_, alpha_true);
  if (debug) fprintf(stderr, "* %6.4f %6.4f %6.4f\n",alpha_star, alpha_star_smooth_, alpha_star_limited);

  if (!ref_.RunningPlan() &&
      fabs(DeltaOldNewRad(alpha_star_limited, prev_alpha_star_limited_)) >
          Deg2Rad(20)) {
    // The heading just jumped by a lot, need a new plan.
    double new_gamma_sail;
    double delta_gamma_sail;
    maneuver_type = FindManeuverType(prev_alpha_star_limited_,
                                     alpha_star_limited,
                                     alpha_true);
    NewGammaSailWithOldGammaSail(alpha_true, mag_true,
                 phi_z_boat, mag_boat,
                 alpha_star_limited,
                 old_gamma_sail,
                 maneuver_type,
                 sail_controller_,
                 &new_gamma_sail,
                 &delta_gamma_sail);
    if (true) {
      ref_.SetReferenceValues(prev_alpha_star_limited_, old_gamma_sail);
      ref_.NewPlan(alpha_star_limited,
                   delta_gamma_sail,
                   mag_boat);
    }
    prev_alpha_star_limited_ = alpha_star_limited;
    ref_.GetReferenceValues(phi_z_star, omega_z_star, gamma_sail_star);
  } else if (ref_.RunningPlan()) {
    ref_.GetReferenceValues(phi_z_star, omega_z_star, gamma_sail_star);
    // We don't accept new reference values here, so we also don't follow up
    // with the previous one.
  } else {
    // Normal case, minor changes of the desired heading.
    *phi_z_star = alpha_star_limited;
    *omega_z_star = 0;
    *gamma_sail_star =
        sail_controller_->BestStabilizedGammaSail(angle_app, mag_app);
    prev_alpha_star_limited_ = alpha_star_limited;
  }
  return maneuver_type;
}

bool NormalController::Tacking() {
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
