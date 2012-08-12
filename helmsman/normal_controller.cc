// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "helmsman/normal_controller.h"

#include <math.h>
#include <stdint.h>
#include <time.h>
#include "common/delta_angle.h"
#include "common/normalize.h"
#include "common/now.h"
#include "common/limit_rate.h"
#include "common/sign.h"
#include "common/polar_diagram.h"
#include "common/unknown.h"
#include "helmsman/apparent.h"
#include "helmsman/new_gamma_sail.h"
#include "helmsman/sampling_period.h"

extern int debug;
// Do not exaggerate here. A big value here means that the sail turns rather fast
// and might actually brake the motion.
static const double kTackOvershootRad = 12 / 180.0 * M_PI;

NormalController::NormalController(RudderController* rudder_controller,
                                   SailController* sail_controller)
   : rudder_controller_(rudder_controller),
     sail_controller_(sail_controller),
     alpha_star_rate_limit_(Deg2Rad(4)),  // 4 deg/s
     old_phi_z_star_(0),
     give_up_counter_(0),
     start_time_ms_(now_ms()),
     trap2_(999),
     prev_offset_(0),
     fallen_off_(0),
     maneuver_type_(kChange) {
  if (debug) fprintf(stderr, "NormalController::Entry time  %lld s\n",
                     start_time_ms_ / 1000);
  if (debug) fprintf(stderr, "NormalController::rate limit  %lf rad/s, %lf deg/s\n",
                     alpha_star_rate_limit_, Rad2Deg(alpha_star_rate_limit_));
  CHECK_EQ(999, trap2_);
}

NormalController::~NormalController() {}

// Entry needs to set all variable states.
void NormalController::Entry(const ControllerInput& in,
                             const FilteredMeasurements& filtered) {
  // Make sure we get the right idea of the direction change when we come
  // from another state.
  old_phi_z_star_ = SymmetricRad(filtered.phi_z_boat);
  double gamma_sail =
      sail_controller_->BestGammaSail(filtered.angle_app, filtered.mag_app);
  sail_controller_->SetAlphaSign(SignNotZero(gamma_sail));
  ref_.SetReferenceValues(old_phi_z_star_, in.drives.gamma_sail_rad);
  give_up_counter_ = 0;
  start_time_ms_ = now_ms();
  if (debug) {
    fprintf(stderr, "NormalController::Entry old_phi_z_star_: %6.1lf deg\n",  Rad2Deg(old_phi_z_star_));
    fprintf(stderr, "Entry Time: %10.1lf s\n", (double)start_time_ms_ / 1000);
  }
  rudder_controller_->Reset();
}

void NormalController::Run(const ControllerInput& in,
                           const FilteredMeasurements& filtered,
                           ControllerOutput* out) {
  if (debug) {
    fprintf(stderr, "------------NormalController::Run----------\n");
    fprintf(stderr, "Actuals: True %6.1lf deg %6.1lf m/s ", Rad2Deg(filtered.alpha_true), filtered.mag_true);
    fprintf(stderr, "Boat %6.1lf deg %6.1lf m/s ", Rad2Deg(filtered.phi_z_boat), filtered.mag_boat);
    fprintf(stderr, "App. %6.1lf deg %6.1lf m/s\n", Rad2Deg(filtered.angle_app), filtered.mag_app);
  }
  CHECK_EQ(999, trap2_);  // Triggers at incomplete compilation errors.
  double phi_star;
  double omega_star;
  double gamma_sail_star;
  // maneuver is set just once to kJibe or kTack, when the maneuver starts.
  // TODO: Put out->status updates into this method.
  ManeuverType maneuver =
      ShapeReferenceValue(SymmetricRad(in.alpha_star_rad),
                          SymmetricRad(filtered.alpha_true), filtered.mag_true,
                          SymmetricRad(filtered.phi_z_boat), filtered.mag_boat,
                          SymmetricRad(filtered.angle_app),  filtered.mag_app,
                          in.drives.gamma_sail_rad,
                          &phi_star,
                          &omega_star,
                          &gamma_sail_star);
  if (maneuver == kTack) {
    out->status.tacks++;
  } else if (maneuver == kJibe) {
    out->status.jibes++;
  }

  if (debug) fprintf(stderr, "IntRef: %6.2lf %6.2lf\n", Rad2Deg(phi_star), Rad2Deg(omega_star));

  double gamma_rudder_star;

  // The boat speed is an unreliable measurement value. It has big systematic and stochastic errors and is
  // therefore filtered and clipped in the filter block. The NormalController can work only
  // with postive boat speeds, but at the transition from the InitialController the very slowly
  // boat speed may be still negative. We simply clip the speed here. If the speed gets too low then
  // we'll leave the NormalController anyway (see GiveUp() ).
  double positive_speed = std::max(0.25, filtered.mag_boat);
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
  if (debug) fprintf(stderr, "Controls: %6.2lf %6.2lf\n\n", Rad2Deg(gamma_rudder_star), Rad2Deg(gamma_sail_star));
}

void NormalController::Exit() {
  if (debug) fprintf(stderr, " NormalController::Exit\n");
}

bool NormalController::IsJump(double old_direction, double new_direction) {
  // All small direction changes are handled directly by the rudder control,
  // with rate limitation for the reference value. Bigger changes and maneuvers
  // require planning because of the synchronized motion of boat and sail.
  // For them IsJump returns true.
  const double JibeZoneWidth = M_PI - JibeZoneRad();
  CHECK(JibeZoneWidth < TackZoneRad());
  return fabs(DeltaOldNewRad(old_direction, new_direction)) >
              1.8 * JibeZoneWidth;
}

// a in [b-eps, b+eps] interval
bool NormalController::Near(double a, double b) {
  const double tolerance = 2 * kSamplingPeriod * alpha_star_rate_limit_;
  return b - tolerance <= a && a <= b + tolerance;
}

// The current bearing is near the TackZone (close hauled) or near the Jibe Zone (broad reach)
// and we will have to do a maneuver.
bool NormalController::IsGoodForManeuver(double old_direction, double new_direction, double angle_true) {
  const double turn = DeltaOldNewRad(old_direction, new_direction);

  const double old_relative = DeltaOldNewRad(angle_true, old_direction);
  // Critical angles to the wind vector. Because the PolarDiagram follows the
  // "wind blows from" convention, we have to turn the tack zone and jibe zone angles.
  const double tack_zone = M_PI - TackZoneRad();
  const double jibe_zone = M_PI - JibeZoneRad();
  if (turn < 0) {
    return Near(old_relative,  jibe_zone) || Near(old_relative, -tack_zone);
  }
  if (turn > 0) {
    return Near(old_relative, -jibe_zone) || Near(old_relative,  tack_zone);
  }
  return false;
}

// Every tack or jibe uses the same angle, i.e. we have standardized jibes and tacks.
double LimitToMinimalManeuver(double old_bearing, double new_bearing,
                              double alpha_true, double maneuver_type,
                              double fallen_off_magnitude) {
  if (maneuver_type == kChange)
    return new_bearing;
  double old_to_wind = DeltaOldNewRad(old_bearing, alpha_true);
  if (maneuver_type == kTack) {
    double tack_angle = 2 * TackZoneRad() + kTackOvershootRad + fallen_off_magnitude;
    if (old_to_wind < 0)
      return SymmetricRad(old_bearing + tack_angle);
    else
      return SymmetricRad(old_bearing - tack_angle);
  }
  if (maneuver_type == kJibe) {
    double jibe_angle = 2 * (M_PI - JibeZoneRad());
    if (old_to_wind < 0)
      return SymmetricRad(old_bearing - jibe_angle);
    else
      return SymmetricRad(old_bearing + jibe_angle);
  }
  CHECK(0);  // Define behaviour for new maneuver type!
  return new_bearing;
}

// Shape the reference value alpha* into something
// sailable, feasible (not too fast changing) and calculate the
// suitable sail angle.
// States: old_phi_z_star_ because phi_z_star must not jump
//   and ref_ because a running plan trumps everything else.
ManeuverType NormalController::ShapeReferenceValue(double alpha_star,
                                                   double alpha_true, double mag_true,
                                                   double phi_z_boat, double mag_boat,
                                                   double angle_app,  double mag_app,
                                                   double old_gamma_sail,
                                                   double* phi_z_star,
                                                   double* omega_z_star,
                                                   double* gamma_sail_star) {
  double phi_z_offset = 0;  // used when sailing close hauled.
  double new_sailable = 0;
  if (debug) fprintf(stderr, "app %6.2lf true %6.2lf old_sail %6.2lf\n", angle_app, alpha_true, old_gamma_sail);

  // 3 cases:
  // If a maneuver is running (tack or jibe or change) we finish that maneuver first
  // else if the new alpha* differs from the old alpha* by more than 20 degrees then
  //     we start a new plan
  // else we restrict alpha* to the sailable range and rate limit it to [-4deg/s, 4deg/s]
  if (ref_.RunningPlan()) {
    // Abort plan if we are far enough.
    if (maneuver_type_ == kTack && ref_.TargetReached(phi_z_boat)) {
      if (debug) fprintf(stderr, "Abort tack\n");
      ref_.Stabilize();
    }
    // The plan includes a stabilization period of a second at the end
    // with constant reference values.
    ref_.GetReferenceValues(phi_z_star, omega_z_star, gamma_sail_star);
    if (maneuver_type_ == kTack) {
      *omega_z_star *= 4;  // So we will always overshoot and abort tacks.
    }
    if (debug) fprintf(stderr, "* %6.2lf %6.2lf %6.2lf\n", alpha_star, alpha_star, *phi_z_star);
  } else {
    // Stay in sailable zone
    new_sailable = BestStableSailableHeading(alpha_star, alpha_true, old_phi_z_star_);
    if (debug) fprintf(stderr, "new sailable: %6.2lf\n", new_sailable);
    if (IsGoodForManeuver(old_phi_z_star_, new_sailable, alpha_true) &&
        IsJump(old_phi_z_star_, new_sailable)) {
      // We need a new plan ...
      maneuver_type_ = FindManeuverType(old_phi_z_star_,
                                        new_sailable,
                                        alpha_true);
      // Limit new_sailable to the other side of the maneuver zone.
      new_sailable = LimitToMinimalManeuver(old_phi_z_star_, new_sailable,
                                            alpha_true, maneuver_type_,
                                            fabs(fallen_off_));
      if (debug) fprintf(stderr, "Start %s maneuver, from  %lf to %lf degrees\n",
                         ManeuverToString(maneuver_type_), Rad2Deg(old_phi_z_star_), Rad2Deg(new_sailable));
      double new_gamma_sail;
      double delta_gamma_sail;
      NewGammaSail(old_gamma_sail,
                   maneuver_type_,
                   kTackOvershootRad,
                   &new_gamma_sail,
                   &delta_gamma_sail);
      sail_controller_->SetAlphaSign(SignNotZero(new_gamma_sail));

      ref_.SetReferenceValues(old_phi_z_star_, old_gamma_sail);
      ref_.NewPlan(new_sailable, delta_gamma_sail);
      ref_.GetReferenceValues(phi_z_star, omega_z_star, gamma_sail_star);
    } else {
      // Normal case, minor changes of the desired heading.
      // Rate limit alpha_star.
      double limited = old_phi_z_star_;
      LimitRateWrapRad(new_sailable, alpha_star_rate_limit_ * kSamplingPeriod, &limited);
      *phi_z_star = limited;
      // Feed forward of the rotation speed helps, but if the boats speed
      // is estimated too low, we get a big overshoot due to exaggerated gamma_0
      // values.
      *omega_z_star = 0;
      // The apparent wind data are filtered to suppress noise in
      // the sail drive reference value.
      *gamma_sail_star =
            sail_controller_->StableGammaSail(alpha_true, mag_true,
                                              angle_app, mag_app,
                                              phi_z_boat,
                                              &phi_z_offset);

      // TODO sail drive lazyness
    }
  }

  old_phi_z_star_ = *phi_z_star;
  fallen_off_ = FilterOffset(phi_z_offset);
  *phi_z_star += fallen_off_;  // so the offset is a temporary thing
  if (debug) fprintf(stderr, "* %6.2lf %6.2lf %6.2lf\n", alpha_star, new_sailable, *phi_z_star);
  return maneuver_type_;
}

double NormalController::FilterOffset(double offset) {
  const double decay = Deg2Rad(0.1) * kSamplingPeriod;
  if (Sign(offset) != 0 && Sign(offset) != Sign(prev_offset_)) {
    prev_offset_ = offset;
    return offset;
  }
  if (offset > 0 || prev_offset_ > 0) {
    if (prev_offset_ > 0)
      prev_offset_ -= decay;
    if (offset > prev_offset_)
      prev_offset_ = offset;
  }
  if (offset < 0 || prev_offset_ < 0) {
    if (prev_offset_ < 0)
      prev_offset_ += decay;
    if (offset < prev_offset_)
      prev_offset_ = offset;
  }
  return prev_offset_;
}

bool NormalController::TackingOrJibing() {
  return ref_.RunningPlan();
}

bool NormalController::GiveUp(const ControllerInput& in,
                              const FilteredMeasurements& filtered) {
  // Got stuck for more than 2 minutes.
  // If the drift caused by stream is too big, we will not detect loss of control here
  // because our speed over ground is e.g. 0.5 knots.
  // Abort every 2 hours? Abort if epsilon is too big?
  // TODO: Contemplate this!
  if (filtered.mag_boat < 0.03)
    ++give_up_counter_;
  else
    give_up_counter_ = 0;
  return give_up_counter_ > 120.0 / kSamplingPeriod;  // The speed is filtered with 60s and rather imprecise.
}

double NormalController::NowSeconds() {
  return (now_ms() - start_time_ms_) / 1000.0;
}

double NormalController::RateLimit() {
  return alpha_star_rate_limit_;
}
