// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

// The reference values for heading and sail control can be set directly
// if they change just a little bit.
// For bigger changes or maneuvers we need a transition plan and coordination
// between boat heading and sail.
// Besides that omega_z* is also generated as input to forward control
// and rudder linearization. 

#ifndef HELMSMAN_REFERENCE_VALUES_H
#define HELMSMAN_REFERENCE_VALUES_H

class ReferenceValues {
 public:
  ReferenceValues();

// For small changes or overriding running plans.
// This method also aborts any running plan.
// omega_z* is implicitly 0.	
void SetReferenceValues(double phi_z_star, double gamma_sail_star);

// For bigger changes and maneuvers:
// Call NewPlan()

// In both cases get the reference values by periodic calls to 
// GetReferenceValues
void GetReferenceValues(double* phi_z_star,
                        double* omega_z_star,
                        double* gamma_sail_star);

// Do a turn of the boat (a normal change of direction, a tack or a jibe) 
// according to this plan:
// 1. Accelerate for 1/6 of the time,
// 2. keep the rotational speed
//    constant for 2/3 of the total time T and 
// 3. finally decelerate for 1/6 of the time. 
// Make a feasible plan that allows to turn the sail during the turn and
// is feasible with the current rudder forces (speed dependant).
// phi_z_1: The new final reference value for the boats heading
// delta_gamma_sail: The difference of the new sail angle to the current one.
// speed_m_s: The boat speed through the water.
void NewPlan(double phi_z_1,
             double delta_gamma_sail,  // gamma sail angles are not normalized
             double speed_m_s);

// returns true if there is a running transition.
bool RunningPlan();

 private:
  int tick_;
  int all_ticks_;

  double acc_;
  double phi_z_;
  double gamma_sail_;

  double omega_;
  double omega_sail_increment_;

  double phi_z_final_;
  double gamma_sail_final_;
};

#endif  // HELMSMAN_REFERENCE_VALUES_H
