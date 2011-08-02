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

// Clamping of waterflow angle. For small speeds the waterflow angle approaches
// infinity, so we have to limit it. 20 degrees is an improved guess based on the
// length from COG to rudder and an estimated turning circle diameter of
// the boat of 20m plus some simulated test results.
const double kLimitGamma0 = Deg2Rad(20);

RudderController::RudderController() : limited_(0), eps_integral_phi_(0) {}

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
  //printf("TorqueToGammaRudder:%6.4f -> %6.4fdeg %d\n",
  //       torque, Rad2Deg(*gamma_rudder_rad), *limited);
}

void RudderController::Control(double phi_star,
                               double omega_star,
                               double phi,
                               double omega,
                               double speed,
                               double* gamma_rudder_rad) {
  // Make control error epsilon for all 3 states omega, phi and int_phi.
  double eps_omega = omega_star - omega;
  // Normalization is essential to cope with going through 180 degrees.
  double eps_phi = SymmetricRad(phi_star - phi);

  // Anti-wind-up (integrator does not fill further, if the
  // control output is at it limits already)
  // reference value for int_phi is always 0.
  /* Good AWU 
  if ((eps_phi < 0 && limited_ <= 0) ||
      (eps_phi > 0 && limited_ >= 0))
    eps_integral_phi_ += kSamplingPeriod * eps_phi;
  const double int_limit = 1;
  if (eps_integral_phi_ > int_limit)
    eps_integral_phi_ = int_limit;
  if (eps_integral_phi_ < -int_limit)
    eps_integral_phi_ = -int_limit;
  if (fabs(eps_phi) > M_PI/2)
    eps_integral_phi_ = 0;  

  */
  
  if (eps_phi * limited_ <= 0)
    eps_integral_phi_ += kSamplingPeriod * eps_phi;

  
  const double int_limit = 1;
  if (eps_integral_phi_ > int_limit)
    eps_integral_phi_ = int_limit;
  if (eps_integral_phi_ < -int_limit)
    eps_integral_phi_ = -int_limit;
  if (fabs(eps_phi) > M_PI/2)
    eps_integral_phi_ = 0;  
  

  // printf("eps_phi %6.4f %6.4f %d \n",  eps_phi, eps_integral_phi_, limited_);
  // Calculate torque around the z-axis which corrects the control error.
  // State vector: [omega, phi, int_phi]
  double torque = eps_omega         * kStateFeedback1 +
                  eps_phi           * kStateFeedback2 +
                  eps_integral_phi_ * kStateFeedback3;

  // Known disturbance torques (from sail)
  double disturbance_torque = 0;
  torque -= disturbance_torque;

  // linearize output
  double gamma_rudder;
  TorqueToGammaRudder(torque, speed, &gamma_rudder, &limited_);
 
  // Relative angle of water flow due to speed and rotation.
  // We are using the reference value of the rotation speed here instead of the
  // actual value because it is the more reliable signal.
  // If the boat goes with a certain speed and turns with omega_star
  // then a rudder with gamma_0 sees an angle of attack of zero.
  // axis z points down, positive omega turns the boat to starboard,d
  // a positive omega must create a negative gamma_0.
  // For low speeds the angle gets too big and is clamped.
  double gamma_0 = atan2(omega_star * kLeverR, speed);
  if (speed < 0) {
    // The rudder is used in a reversed position in this situation.
    gamma_0 = SymmetricRad(gamma_0 - M_PI);
  }

  if (gamma_0 > kLimitGamma0) {
    gamma_0 = kLimitGamma0;
    if (debug) fprintf(stderr, "+limit gamma 0");
  }
  if (gamma_0 < -kLimitGamma0) {
    gamma_0 = -kLimitGamma0;
    if (debug) fprintf(stderr, "-limit gamma 0");
  }
  
  // need negative gamma for positive torque
  *gamma_rudder_rad = -(gamma_rudder + gamma_0);
}

void RudderController::Reset() {
  //printf("\nReset.\n");
  eps_integral_phi_ = 0;
  limited_ = 0;
};
