// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011




#include "helmsman/initial_controller.h"  

#include <stdio.h>

#include "lib/fm/log.h"
#include "common/delta_angle.h"
#include "common/polar_diagram.h"
#include "common/sign.h"
#include "helmsman/sampling_period.h"  
#include "helmsman/wind_strength.h"

namespace {
const double kReverseMotionSailAngle = Deg2Rad(90);
const double kReverseMotionRudderAngle = Deg2Rad(15);
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
  double gamma_sail = 0;
  double gamma_rudder = 0;
  out->Reset();
  if (!filtered.valid) {
    return;
  }
  if (!in.drives.homed_sail ||
      (!in.drives.homed_rudder_left && !in.drives.homed_rudder_right)) {
    FM_LOG_INFO("Drives not ready");
  }

  double angle_app = SymmetricRad(filtered.angle_app);

  switch (phase_) {
    case SLEEP:
      gamma_sail = 0;
      gamma_rudder = 0;
      // wait 10s for all filters to settle
      if (++count_ > 10.0 / kSamplingPeriod &&
          in.drives.homed_sail &&
          in.drives.homed_rudder_left &&
          in.drives.homed_rudder_right) {
        count_ = 0;
        phase_ = TURTLE;
        // Decide which way to go.
        sign_ = SignNotZero(angle_app);
        FM_LOG_INFO("SLEEP to TURTLE %d\n", sign_);
      }
      break;
    case TURTLE:
      // Turn into a sailable direction if necessary.
      if (fabs(angle_app) < kSailableLimit &&
          WindStrength(kCalmWind, filtered.mag_app) != kCalmWind) {
        phase_ = KOGGE;
        count_ = 0;
        FM_LOG_INFO("TURTLE to KOGGE %d\n", sign_);
        break;
      }
      gamma_rudder = kReverseMotionRudderAngle * sign_;
      gamma_sail = -kReverseMotionSailAngle * sign_;
      break;
    case KOGGE:
      gamma_sail = sail_controller_->BestStabilizedGammaSail(angle_app, filtered.mag_app);
      // Force the apparent angle to 90 degrees (beam reach).
      if (fabs(angle_app) > kSailableLimit)
        gamma_rudder = -Sign(angle_app) * kBangBangRudderAngle;
      else  
        gamma_rudder = 0;
      break; 
  }
  out->drives_reference.gamma_sail_star_rad = gamma_sail;
  out->drives_reference.gamma_rudder_star_left_rad = gamma_rudder;
  out->drives_reference.gamma_rudder_star_right_rad = gamma_rudder;
}


InitialController::~InitialController() {}

void InitialController::Entry(const ControllerInput& in,
                              const FilteredMeasurements& filtered) {
  Reset();
}

void InitialController::Exit() {}

bool InitialController::Done() {
  return phase_ == KOGGE && 
         (++count_ >  10.0 / kSamplingPeriod);
}
