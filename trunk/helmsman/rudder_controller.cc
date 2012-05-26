// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/rudder_controller.h"

#include <math.h>
#include <stdio.h>
#include "common/convert.h"
#include "common/normalize.h"
#include "helmsman/boat.h"  // constants from simulation/boat.m
#include "helmsman/c_lift_to_rudder_angle.h"  // rudder hydrodynamics
#include "helmsman/sampling_period.h"
#include "helmsman/rudder_controller_const.h"

extern int debug;
static int debug_controller = 0;

// Clamping of waterflow angle. For small speeds the waterflow angle approaches
// infinity, so we have to limit it. 20 degrees is an improved guess based on the
// length from COG to rudder and an estimated turning circle diameter of
// the boat of 20m plus some simulated test results.
const double kLimitGamma0 = Deg2Rad(20);


// for tests
void RudderController::SetFeedback(double k1, double k2, double k3, bool feed_forward) {
  state_feedback_[0] = k1;
  state_feedback_[1] = k2;
  state_feedback_[2] = k3;
  feed_forward_ = feed_forward;
}

RudderController::RudderController()
    : limited_(0), eps_integral_phi_(0), feed_forward_(true) {
  state_feedback_[0] = kStateFeedback1;
  state_feedback_[1] = kStateFeedback2;
  state_feedback_[2] = kStateFeedback3;
}

// Linearize the plant
void RudderController::TorqueToGammaRudder(double torque,
                                           double speed,
                                           double* gamma_rudder_rad,
                                           int* limited) {
  // torque = F * kLeverR
  double force = torque / kLeverR;
  // F = kNumberR * kAreaR * (kRhoWater / 2) * speed^2 * C_lift(alpha, speed)
  double c_lift = 2 * force / (kNumberR * kAreaR * kRhoWater * speed * speed);
  CLiftToRudderAngle(c_lift, speed, gamma_rudder_rad, limited);
  if (debug_controller) fprintf(stderr, "TorqueToGammaRudder:%6.4lf -> %6.4lfdeg %d\n",
                                torque, Rad2Deg(*gamma_rudder_rad), *limited);
}

void RudderController::Control(double phi_star,
                               double omega_star,
                               double phi,
                               double omega,
                               double speed,
                               double* gamma_rudder_rad) {
  if (debug) fprintf(stderr, "phi* %lf phi %lf speed %lf\n", Rad2Deg(phi_star), Rad2Deg(phi), speed);
  // Make control error epsilon for all 3 states omega, phi and int_phi.
  double eps_omega = omega_star - omega;
  // Normalization is essential to cope with going through 180 degrees.
  double eps_phi = SymmetricRad(phi_star - phi);

  // Anti-wind-up (integrator does not fill further, if the
  // control output is at it limits already)
  // reference value for int_phi is always 0.
  if (eps_phi * limited_ <= 0)
    eps_integral_phi_ += kSamplingPeriod * eps_phi;

  const double int_limit = 1;  // radians * second
  if (eps_integral_phi_ > int_limit)
    eps_integral_phi_ = int_limit;
  if (eps_integral_phi_ < -int_limit)
    eps_integral_phi_ = -int_limit;
  if (fabs(eps_phi) > M_PI/2)
    eps_integral_phi_ = 0;


  // Calculate torque around the z-axis which corrects the control error.
  // State vector: [omega, phi, int_phi]
  double torque = eps_omega         * state_feedback_[0] +
                  eps_phi           * state_feedback_[1] +
                  eps_integral_phi_ * state_feedback_[2];

  // Known disturbance torques (from sail)
  double disturbance_torque = 0;
  torque -= disturbance_torque;

  // linearize output
  double gamma_rudder;
  TorqueToGammaRudder(torque, speed, &gamma_rudder, &limited_);
  if (debug_controller)
      fprintf(stderr, "eps_states [%6.4lf %6.4lf %6.4lf] control limited: %d \n",
              eps_omega, eps_phi, eps_integral_phi_, limited_);

  // Relative angle of water flow due to speed and rotation.
  // We are using the reference value of the rotation speed here instead of the
  // actual value because it is the more reliable signal.
  // If the boat goes with a certain speed and turns with omega_star
  // then a rudder with gamma_0 sees an angle of attack of zero.
  // axis z points down, positive omega turns the boat to starboard,
  // a positive omega must create a negative gamma_0.
  // For low speeds the angle gets too big and is clamped.
  double gamma_0 = -atan2(omega_star * kLeverR, speed);
  if (speed < 0) {
    // The rudder profile is used in a reversed fashion in this situation.
    gamma_0 = SymmetricRad(gamma_0 - M_PI);
  }

  if (gamma_0 > kLimitGamma0) {
    gamma_0 = kLimitGamma0;
    if (debug) fprintf(stderr, "+limit gamma 0\n");
  }
  if (gamma_0 < -kLimitGamma0) {
    gamma_0 = -kLimitGamma0;
    if (debug) fprintf(stderr, "-limit gamma 0\n");
  }

  if (feed_forward_) {
    // need negative gamma_rudder for positive torque
    if (debug) fprintf(stderr, "gamma 0 : %g\n", gamma_0);
    *gamma_rudder_rad = gamma_0 - gamma_rudder;
  } else {
    *gamma_rudder_rad = -gamma_rudder;
  }
}

void RudderController::Reset() {
  eps_integral_phi_ = 0;
  limited_ = 0;
};
