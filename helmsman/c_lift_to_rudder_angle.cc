// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011


#include "helmsman/c_lift_to_rudder_angle.h"
#include "helmsman/naca0010.h"
#include "common/sign.h"
#include "common/unknown.h"

namespace naca0010 {

namespace {

double RudderLimit(double speed_m_s) {
  // The least restrictive for exceptional situations.
  // The only harm it can do is speed loss (which is of no concern because we
  // don't know about it because we cannot measure it!).
  if (speed_m_s == kUnknown || speed_m_s < 0)
    return kAlphaLimit4Rad;
  if (speed_m_s < kSpeed1_m_s) {
    return kAlphaLimit1Rad;
  } else if (speed_m_s < kSpeed2_m_s) {
    return kAlphaLimit2Rad;
  } else if (speed_m_s < kSpeed3_m_s) {
    return kAlphaLimit3Rad;
  } else {
    return kAlphaLimit4Rad;
  }
}

}  // namespace

}  // namespace naca0010

// limited is
// -1 for limitation at the lower limit of c_lift
//  0 for the unlimited case
// +1 for a limitation at the positive limit of the c_lift.
void CLiftToRudderAngle(double c_lift,
                        double speed_m_s,
                        double* alpha_rad,
                        int* limited) {
  double limit = naca0010::RudderLimit(speed_m_s);
  double c_lift_per_rad;
  if (speed_m_s != kUnknown && speed_m_s < 0) {
    c_lift_per_rad = naca0010::kCLiftPerRadReverse;
  } else {
    c_lift_per_rad = naca0010::kCLiftPerRad;
  }

  double alpha_unlimited = c_lift / c_lift_per_rad * Sign(speed_m_s);
  if (alpha_unlimited < -limit) {
    *alpha_rad = -limit;
    *limited = -Sign(speed_m_s);
  } else if (alpha_unlimited > limit) {
    *alpha_rad = limit;
    *limited = Sign(speed_m_s);
  } else {
    *alpha_rad = alpha_unlimited;
    *limited = 0;
  }
}