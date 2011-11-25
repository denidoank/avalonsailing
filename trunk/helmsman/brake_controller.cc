// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#include "helmsman/brake_controller.h"

#include "common/unknown.h"  // kUnknown
#include "common/convert.h"
#include "common/limit_rate.h"
#include "common/sign.h"
#include "helmsman/sampling_period.h"


extern int debug;

BrakeController::~BrakeController() {}

void BrakeController::Entry(const ControllerInput& in,
                            const FilteredMeasurements& filtered) {
  count_ = 0;

}


void BrakeController::Run(const ControllerInput& in,
                          const FilteredMeasurements& filtered,
                          ControllerOutput* out) {
  out->Reset();
  if (++count_ < 20 / kSamplingPeriod ) {
    out->drives_reference.gamma_rudder_star_left_rad  =  kRudderBrakeAngleRad;
    out->drives_reference.gamma_rudder_star_right_rad = -kRudderBrakeAngleRad;
    // Turn the sail into the flag position, if the wind direction is known.
    gamma_sail_rad_ = 0;
    if (filtered.angle_app != kUnknown)
      gamma_sail_rad_ = SymmetricRad(filtered.angle_app - M_PI);
    sign_ = SignNotZero(gamma_sail_rad_);
    if (debug) fprintf(stderr, "BrakeController::Run: rudder braked and sail %lf\n", Rad2Deg(gamma_sail_rad_));
  } else {
    // Heave to, the Avalon way.
    out->drives_reference.gamma_rudder_star_left_rad  = -sign_ * Deg2Rad(16);
    out->drives_reference.gamma_rudder_star_right_rad = -sign_ * Deg2Rad(16);
    LimitRateWrapRad(sign_ * M_PI / 2, Deg2Rad(5) * kSamplingPeriod, &gamma_sail_rad_);
    if (debug) fprintf(stderr, "BrakeController::Run: Heave-to and sail %lf\n", Rad2Deg(gamma_sail_rad_));
  }
  out->drives_reference.gamma_sail_star_rad = gamma_sail_rad_;
}
