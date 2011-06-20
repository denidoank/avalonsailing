// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#ifndef HELMSMAN_RUDDER_CONTROL_H
#define HELMSMAN_RUDDER_CONTROL_H

class RudderController {
 public:
  RudderController();
  // phi_z is the angle around the vertical z axis or the heading of the boat.
  // omega_z is the respective rotational speed around the z axis or the yaw
  // rate of the boat.
  // A "star" indicates a reference or desired value, while the name without
  // star is the actual or measured value.
  // This controller consequently processes 2 pairs of reference and actual
  // values, namely for phi_z and omega_z.
  // The speed is needed because the rudder effect is strongly speed dependant
  // and we have to compensate for that to keep the control plants model at
  // least approximately linear. 
  void Control(double phi_z_star_rad,
               double omega_star_rad_s,
               double phi_z_rad,
               double omega_z_rad_s,
               double speed_m_per_s,       // the boat speed through the water 
               double* gamma_rudder_rad);  // the control variable rudder angle
  // Reset integral control output part.
  void Reset();
 private:
  // Linearize the plant
  void TorqueToGammaRudder(double torque_Nm, // desired torque to turn the boat
                           double speed_m_s,   // speed through water
                           double* gamma_rudder_rad,
                           int* limited);       // see limited_

  // Limitation state, i.e. rudder angle at limit
  // -1: at lower limit, 0 unlimited, +1 at upper control limit
  int limited_;
  // Artificial state to achieve a steady state control accuracy
  // Roughly equal to the clamped integral of the heading error,
  // integral of phi.
  double eps_integral_phi_;
};

#endif  // HELMSMAN_RUDDER_CONTROL_H
