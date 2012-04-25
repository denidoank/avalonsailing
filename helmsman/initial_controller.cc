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
const double kReverseMotionSailAngle = Deg2Rad(90);
const double kReverseMotionRudderAngle = Deg2Rad(8);
// Lets sail a while in broad reach.
const double kReferenceAngleApp = Deg2Rad(90);
const double kBangBangRudderAngle = Deg2Rad(5);
const double kSailableLimit = Deg2Rad(130);
const double kMinWindSpeedMPerS = 0.5;
}


InitialController::InitialController(SailController* sail_controller)
    : sail_controller_(sail_controller) {
  Reset();
}
  
void InitialController::InitialController::Reset() {
  phase_ = SLEEP;
  sign_ = 0; 
  count_ = 0;
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
  
  if (debug) {
    fprintf(stderr, "WindSensor: %s\n", in.wind_sensor.ToString().c_str());
    fprintf(stderr, "DriveActuals: %s\n", in.drives.ToString().c_str());
    fprintf(stderr, "AppFiltered: %fdeg %fm/s\n", Rad2Deg(filtered.angle_app), filtered.mag_app);
  }
  
  double angle_app = SymmetricRad(filtered.angle_app);

  switch (phase_) {
    case SLEEP:
      if (debug) fprintf(stderr, "phase SLEEP\n");
      gamma_sail = 0;
      gamma_rudder = 0;
      // wait 5s for all filters to settle
      if (++count_ > 5.0 / kSamplingPeriod &&
          in.drives.homed_sail &&
          (in.drives.homed_rudder_left ||
          in.drives.homed_rudder_right)) {
        count_ = 0;
        phase_ = TURTLE;
        // Decide which way to go.
        sign_ = SignNotZero(angle_app);
        if (debug) fprintf(stderr, "SLEEP to TURTLE %d\n", sign_);
        out->status.inits++;
      }
      break;
    case TURTLE:
      {
        // Turn boat out of irons, if necessary.
        // If the boat points into the wind, turn the sail sideways and
        // put the rudder at a big angle such that the boat turns.
        if (debug) fprintf(stderr, "phase TURTLE\n");
        // For weak winds we need a bearing around a dead run +- 90degrees, to
        // get enough speed in every situation.
        // For stronger winds we have enough momentum pushing back.
        const double run_sector = filtered.mag_app > 5 ? Deg2Rad(120) : Deg2Rad(90);
        // Turn into a sailable direction if necessary.
        if (fabs(angle_app) > run_sector ||
            WindStrength(kCalmWind, filtered.mag_app) == kCalmWind) {
          gamma_rudder = kReverseMotionRudderAngle * sign_;
          gamma_sail = sail_controller_->BestGammaSailForReverseMotion(angle_app, filtered.mag_app);
          break;
        } else {
          phase_ = KOGGE;
          count_ = 0;
          if (debug) fprintf(stderr, "TURTLE to KOGGE %d\n", sign_);
          // intentional falltrough
        }
      }
    case KOGGE:
      if (debug) fprintf(stderr, "phase KOGGE\n");
      gamma_sail = sail_controller_->BestStabilizedGammaSail(angle_app, filtered.mag_app);
      if (fabs(angle_app) > Deg2Rad(120))
        gamma_rudder = -Sign(angle_app) * kBangBangRudderAngle;
      else  
        gamma_rudder = 0;
      ++count_;
      break; 
  }
  out->drives_reference.gamma_sail_star_rad = gamma_sail;
  out->drives_reference.gamma_rudder_star_left_rad = gamma_rudder;
  out->drives_reference.gamma_rudder_star_right_rad = gamma_rudder;

   if (debug) {
     fprintf(stderr, "out gamma_sail:%lf deg\n", Rad2Deg(gamma_sail));
     fprintf(stderr, "out gamma_rudder_star left/right:%lf deg\n", Rad2Deg(gamma_rudder));
   }
}


InitialController::~InitialController() {}

void InitialController::Entry(const ControllerInput& in,
                              const FilteredMeasurements& filtered) {
  Reset();
}

void InitialController::Exit() {}

bool InitialController::Done() {
  const bool done = (phase_ == KOGGE) && (count_ >  15.0 / kSamplingPeriod);
  if (done && debug) fprintf(stderr, "InitialController::Done\n");
  return done;
}
