// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "helmsman/normal_controller.h"

#include <math.h>
#include <stdint.h>
#include <time.h>
#include "common/apparent.h"
#include "common/delta_angle.h"
#include "common/normalize.h"
#include "common/now.h"
#include "common/limit_rate.h"
#include "common/sign.h"
#include "common/polar_diagram.h"
#include "common/unknown.h"
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
     maneuver_type_(kChange),
     wind_strength_apparent_(kCalmWind),
     prev_sector_(JibePort),
     alpha_star_rate_limited_(0) {
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
  alpha_star_rate_limited_ = old_phi_z_star_;
  // This serves to initialize prev_sector_ only.
  double dummy_target;
  point_of_sail_.SailableHeading(
      filtered.phi_z_boat,  // desired heading alpha*
      filtered.alpha_true,  // true wind vector direction
      filtered.phi_z_boat,  // previous output direction, needed to implement hysteresis
      &prev_sector_,        // sector codes for state handling and maneuver
      &dummy_target);
  //double gamma_sail =
  //    sail_controller_->BestGammaSail(filtered.angle_app, filtered.mag_app);
  sail_controller_->SetAlphaSign(SectorToGammaSign(prev_sector_));
  ref_.SetReferenceValues(old_phi_z_star_, in.drives.gamma_sail_rad);
  give_up_counter_ = 0;
  start_time_ms_ = now_ms();
  if (debug) {
    fprintf(stderr, "NormalController::Entry old_phi_z_star_: %6.1lf deg\n",  Rad2Deg(old_phi_z_star_));
    fprintf(stderr, "Entry Time: %10.1lf s\n", (double)start_time_ms_ / 1000);
  }
  rudder_controller_->Reset();
  point_of_sail_.Reset();
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
  wind_strength_apparent_ = WindStrength(wind_strength_apparent_, filtered.mag_app);

  CHECK_EQ(999, trap2_);  // Triggers at incomplete compilation errors.
  double phi_star;
  double omega_star;
  double gamma_sail_star;
  ShapeReferenceValue(SymmetricRad(in.alpha_star_rad),
                      SymmetricRad(filtered.alpha_true), filtered.mag_true,
                      SymmetricRad(filtered.phi_z_boat), filtered.mag_boat,
                      SymmetricRad(filtered.angle_app),  filtered.mag_app,
                      in.drives.gamma_sail_rad,
                      &phi_star,
                      &omega_star,
                      &gamma_sail_star,
                      out);

  if (debug) fprintf(stderr, "IntRef: %6.2lf %6.2lf\n", Rad2Deg(phi_star), Rad2Deg(omega_star));

  double gamma_rudder_star;

  // The boat speed is an unreliable measurement value. It has big systematic and stochastic errors and is
  // therefore filtered and clipped in the filter block. The NormalController can work only
  // with postive boat speeds, but at the transition from the InitialController the very slowly
  // boat speed may be still negative. We simply clip the speed here. If the speed gets too low then
  // we'll leave the NormalController anyway (see GiveUp() ).
  // For the rudder controller the speed through the water is relevant only.
  double positive_speed = std::max(0.2, filtered.mag_boat);
  positive_speed = std::min(3.2, positive_speed);  // 4.8knots max speed = 2.46m/s
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
  epsilon_ = SymmetricRad(phi_star - filtered.phi_z_boat);
  out->drives_reference.gamma_rudder_star_left_rad  = gamma_rudder_star;
  out->drives_reference.gamma_rudder_star_right_rad = gamma_rudder_star;
  out->drives_reference.gamma_sail_star_rad = gamma_sail_star;
  if (debug) fprintf(stderr, "Controls: %6.2lf %6.2lf\n\n", Rad2Deg(gamma_rudder_star), Rad2Deg(gamma_sail_star));
}

void NormalController::Exit() {
  if (debug) fprintf(stderr, " NormalController::Exit\n");
}

double AddOvershoot(double old_bearing,
                    double new_bearing,
                    double maneuver_type) {
  int sign = SignNotZero(DeltaOldNewRad(old_bearing, new_bearing));
  if (maneuver_type == kTack) {
    return new_bearing + sign * kTackOvershootRad;
  }
  if (maneuver_type == kJibe) {
    return new_bearing;
  }
  CHECK(maneuver_type != kChange);
  CHECK(0);  // Define behaviour for new maneuver type!
  return new_bearing;
}

// Shape the reference value alpha* into something
// sailable, feasible (not too fast changing) and calculate the
// suitable sail angle.
// States: old_phi_z_star_ because phi_z_star must not jump
//   and ref_ because a running plan trumps everything else.
void NormalController::ShapeReferenceValue(double alpha_star,
                                           double alpha_true, double mag_true,
                                           double phi_z_boat, double mag_boat,
                                           double angle_app,  double mag_app,
                                           double old_gamma_sail,
                                           double* phi_z_star,
                                           double* omega_z_star,
                                           double* gamma_sail_star,
                                           ControllerOutput* out) {
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
    alpha_star_rate_limited_ = *phi_z_star;
    if (debug) fprintf(stderr, "* %6.2lf %6.2lf %6.2lf\n", alpha_star, alpha_star, *phi_z_star);
  } else {
    // Rate limit alpha_star.
    LimitRateWrapRad(alpha_star, alpha_star_rate_limit_ * kSamplingPeriod, &alpha_star_rate_limited_);
    // Stay in sailable zone
    SectorT sector;
    double maneuver_target;
    new_sailable = point_of_sail_.SailableHeading(
        alpha_star_rate_limited_,  // desired heading alpha*
        alpha_true,   // true wind vector direction
        old_phi_z_star_,  // previous output direction, needed to implement hysteresis
        &sector,      // sector codes for state handling and maneuver
        &maneuver_target);
    if (sector == prev_sector_) {
      new_sailable += point_of_sail_.AntiWindGust(sector,
                                                  angle_app,  // apparent wind vector direction
                                                  mag_app);   // apparent wind vector magnitude
    }
    maneuver_type_ = SectorToManeuver(sector);
    sail_controller_->SetAlphaSign(SectorToGammaSign(sector));

    if (debug) fprintf(stderr, "new sailable: %6.2lf\n", new_sailable);

    if (maneuver_type_ == kTack || maneuver_type_ == kJibe) {
      // We need a new plan ...
      new_sailable = AddOvershoot(old_phi_z_star_, maneuver_target, maneuver_type_);
      if (debug) fprintf(stderr, "Start %s maneuver, from  %lf to %lf degrees\n",
                         ManeuverToString(maneuver_type_), Rad2Deg(old_phi_z_star_), Rad2Deg(new_sailable));
      double new_gamma_sail;
      double delta_gamma_sail;
      NewGammaSail(old_gamma_sail,
                   maneuver_type_,
                   kTackOvershootRad,
                   &new_gamma_sail,
                   &delta_gamma_sail);

      ref_.SetReferenceValues(old_phi_z_star_, old_gamma_sail);
      ref_.NewPlan(new_sailable, delta_gamma_sail);
      switch (maneuver_type_) {
        case kTack:
          out->status.tacks++;
          break;
        case kJibe:
          out->status.jibes++;
          break;
        default:
          CHECK(0);
      }

      ref_.GetReferenceValues(phi_z_star, omega_z_star, gamma_sail_star);
    } else {
      // Normal case, minor changes of the desired heading.
      *phi_z_star = new_sailable;
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
    prev_sector_ = sector;
  }

  old_phi_z_star_ = *phi_z_star;
  if (debug) fprintf(stderr, "* %6.2lf %6.2lf %6.2lf\n", alpha_star, new_sailable, *phi_z_star);
}

bool NormalController::TackingOrJibing() {
  return ref_.RunningPlan();
}

bool NormalController::GiveUp(const ControllerInput& in,
                              const FilteredMeasurements& filtered) {
  const double give_up_limit_seconds = 30.0;
  // Abort if we got stuck for more than 30s.
  // const double bearing_deviation_limit = Deg2Rad(35);
  // If the drift caused by stream is too big, we will not detect loss of control here
  // because our speed over ground is e.g. 0.5 knots.
  // Abort every 2 hours? Abort if epsilon is too big?
  // TODO: Contemplate this!
  // The speed is filtered and rather imprecise.
  // TODO Check other options like
  // (wind_strength_apparent_ != kCalmWind && fabs(epsilon_) > bearing_deviation_limit))
  // This aborted too early during 2012_08_28 tests.
  if (wind_strength_apparent_ != kCalmWind && filtered.mag_boat < 0.03)
    ++give_up_counter_;
  else
    give_up_counter_ = 0;
  if (give_up_counter_ > give_up_limit_seconds / kSamplingPeriod) {
    if (debug) fprintf(stderr, "NormalController GiveUp mag_boat %6.2lf eps %6.2lf\n", filtered.mag_boat, epsilon_);
    return true;
  } else {
    return false;
  }
}

double NormalController::NowSeconds() {
  return (now_ms() - start_time_ms_) / 1000.0;
}

double NormalController::RateLimit() const{
  return alpha_star_rate_limit_;
}

// uses prev_sector_
ManeuverType NormalController::SectorToManeuver(SectorT sector) {
  if (debug) fprintf(stderr, "sector %d\n", int(sector));
  if ((prev_sector_ == TackPort && sector == TackStar)  ||
      (prev_sector_ == TackStar && sector == TackPort)) {
    return kTack;
  }
  if ((prev_sector_ == JibePort && sector == JibeStar)  ||
      (prev_sector_ == JibeStar && sector == JibePort)) {
    return kJibe;
  }
  return kChange;
}

int NormalController::SectorToGammaSign(SectorT sector) {
  if (sector == TackStar || sector == ReachStar || sector == JibeStar) {
    return -1;
  } else {
    return 1;
  }
}

void NormalController::CountManeuvers(ControllerOutput* out) {
  switch (maneuver_type_) {
    case kTack:
      out->status.tacks++;
      break;
    case kJibe:
      out->status.jibes++;
      break;
    default:
      break;
  }
}
