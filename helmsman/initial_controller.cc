// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/initial_controller.h"  

#include <stdio.h>

#include "common/delta_angle.h"
#include "common/polar_diagram.h"
#include "common/sign.h"
#include "helmsman/sampling_period.h"  
#include "helmsman/wind_strength.h"
#include "helmsman/wind_sensor.h"

extern int debug; // global shared

namespace {
const double kReverseMotionSailAngle = Deg2Rad(95);
const double kReverseMotionRudderAngle = Deg2Rad(8);
// Lets sail a while in broad reach.
const double kReferenceAngleApp = Deg2Rad(90);
const double kBangBangRudderAngle = Deg2Rad(5);
const double kSailableLimit = Deg2Rad(130);
const double kMinWindSpeedMPerS = 0.5;
// TURTLE STATE
// For weak winds we need a bearing around a dead run +- 90degrees, to
// get enough speed in every situation. Angles up to 120 degrees would
// work most of the time for stronger winds, but there are initial states
// (app. wind -103.98deg, v = -0.63m/s) where the boat gets caught in the
// KOGGE state.
const double kRunSector = Deg2Rad(85);  // Definitively downwind.

const double kMinimumSpeed = 0.15;  // m/s, needed to leave the KOGGE state.
}


InitialController::InitialController(SailController* sail_controller)
    : sail_controller_(sail_controller),
      positive_speed_(false),
      kogge_time_(60) {
  Reset();
}
  
void InitialController::InitialController::Reset() {
  phase_ = SLEEP;
  sign_ = 0; 
  count_ = 0;
  kogge_time_ = 60;
}

void InitialController::Run(const ControllerInput& in,
                            const FilteredMeasurements& filtered,
                            ControllerOutput* out) {

  if (debug) fprintf(stderr, "----InitialController::Run-------\n");

  double gamma_sail = 0;
  double gamma_rudder = 0;
  out->Reset();
  if (!filtered.valid) {
    return;
  }
  if (!in.drives.homed_sail ||
      (!in.drives.homed_rudder_left && !in.drives.homed_rudder_right)) {
    if (debug) fprintf(stderr, "Drives not ready\n %s %s %s",
		      in.drives.homed_rudder_left ? "" : "sail",
		      in.drives.homed_rudder_left ? "" : "left",
		      in.drives.homed_rudder_left ? "" : "right");
  }

  // The delay of 20s could be smaller but if we need to tack immediately
  // after leaving the InitialController then the boat needs some momentum
  // and the speed measurement filter should have a stabilized value.
  // This depends on the quality of the IMU speed calculation. Therefore
  // the more conservative waiting time of 20s.
  kogge_time_ = filtered.mag_app > 4.0 ?
      20.0 : // no time to waste
      60.0;  // for weak wind

  positive_speed_ = filtered.mag_boat > kMinimumSpeed;
  if (debug) {
    fprintf(stderr, "WindSensor: %s\n", in.wind_sensor.ToString().c_str());
    fprintf(stderr, "DriveActuals: %s\n", in.drives.ToString().c_str());
    if (filtered.valid_app_wind)
      fprintf(stderr, "AppFiltered: %6.2lf deg %6.2lf m/s\n", Rad2Deg(filtered.angle_app), filtered.mag_app);
    else
      fprintf(stderr, "AppFiltered: Magnitude %6.2lf m/s\n", filtered.mag_app);
  }
  
  double angle_app = SymmetricRad(filtered.angle_app);
  Polar apparent_wind(angle_app, filtered.mag_app);

  switch (phase_) {
    case SLEEP:
      if (debug) fprintf(stderr, "phase SLEEP\n");
      gamma_sail = 0;
      gamma_rudder = 0;
      if (filtered.valid_app_wind &&
          in.drives.homed_sail &&
          (in.drives.homed_rudder_left ||
           in.drives.homed_rudder_right)) {
        count_ = 0;
        phase_ = TURTLE;
        // Decide which way to go.
        sign_ = SignNotZero(angle_app);
        sail_controller_->SetAppSign(sign_);
        if (debug) fprintf(stderr, "SLEEP to TURTLE %d\n", sign_);
        out->status.inits++;
      }
      break;
    case TURTLE:
      // Turn boat out of irons, if necessary.
      // If the boat points into the wind, turn the sail sideways and
      // put the rudder at a big angle such that the boat turns.
      if (debug) fprintf(stderr, "phase TURTLE\n");
      // Turn into a sailable direction if necessary.
      if (fabs(angle_app) <= kRunSector) {
        phase_ = KOGGE;
        count_ = 0;
        if (debug) fprintf(stderr, "TURTLE to KOGGE %d\n", sign_);
      }
      gamma_rudder = kReverseMotionRudderAngle * sign_;
      gamma_sail = sail_controller_->ReverseGammaSailFromApparent(apparent_wind).rad();
      break;
    case KOGGE:
      if (debug) fprintf(stderr, "phase KOGGE\n");
      sail_controller_->SetAppSign(sign_);  // which side the wind is coming from.
      gamma_sail = sail_controller_->StableGammaSailFromApparent(apparent_wind).rad();
      if (fabs(angle_app) > Deg2Rad(120))
        gamma_rudder = -Sign(angle_app) * kBangBangRudderAngle;
      else  
        gamma_rudder = 0;
      // Abort if we don't get enough speed.
      if (count_ > 3 * kogge_time_ / kSamplingPeriod &&
          filtered.mag_app > 0.5) {
        count_ = 0;
        phase_ = TURTLE;
        // Decide which way to go.
        sign_ = SignNotZero(angle_app);
        sail_controller_->SetAppSign(sign_);
        if (debug) fprintf(stderr, "KOGGE to TURTLE %d\n", sign_);
      }
      break;
  }
  ++count_;
  out->drives_reference.gamma_sail_star_rad = gamma_sail;
  out->drives_reference.gamma_rudder_star_left_rad = gamma_rudder;
  out->drives_reference.gamma_rudder_star_right_rad = gamma_rudder;

  if (debug) {
    fprintf(stderr, "out gamma_sail:%6.2lf deg\n", Rad2Deg(gamma_sail));
    fprintf(stderr, "out gamma_rudder_star left/right:%6.2lf deg\n", Rad2Deg(gamma_rudder));
  }
}


InitialController::~InitialController() {}

void InitialController::Entry(const ControllerInput& in,
                              const FilteredMeasurements& filtered) {
  Reset();
}

void InitialController::Exit() {}

bool InitialController::Done() {
  const bool done = (phase_ == KOGGE) &&
                    (count_ > kogge_time_ / kSamplingPeriod) &&
                     positive_speed_;
  if (done && debug) fprintf(stderr, "InitialController::Done\n");
  return done;
}
