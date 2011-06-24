// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/new_gamma_sail.h"

#include "common/delta_angle.h"
#include "common/normalize.h"
#include "common/polar.h"
#include "common/sign.h"
#include "helmsman/apparent.h"

#include <stdio.h>

void NewGammaSail(double alpha_true, double mag_true,
                  double alpha_boat, double mag_boat,
                  double new_alpha_boat,
                  SailController* sail_controller,
                  double* new_gamma_sail,
                  double* delta_gamma_sail,
                  ManeuverType* maneuver_type) {
  new_alpha_boat = SymmetricRad(new_alpha_boat);               
  // We assume that the boat speed is constant during tack (or a little less)
  double new_mag_boat = 1.0 * mag_boat;
  double alpha_app;
  double mag_app;
  Apparent(alpha_true, mag_true, alpha_boat, mag_boat, &alpha_app, &mag_app);
  // apparent.Print("\nold_apparent");
  double old_gamma_sail = sail_controller->BestGammaSail(alpha_app, mag_app);
  // printf("old gamma: %6.2g deg\n", Rad2Deg(old_gamma_sail));   
  double new_alpha_app;
  double new_mag_app;
  Apparent(alpha_true, mag_true, new_alpha_boat, new_mag_boat, &new_alpha_app, &new_mag_app);

  // printf("alpha_app %6.2f deg , new_alpha_app %6.2f\n", Rad2Deg(alpha_app), Rad2Deg(new_alpha_app));   
 
  *maneuver_type = FindManeuverType(alpha_app, new_alpha_app);
  *new_gamma_sail = sail_controller->
      BestGammaSail(new_alpha_app, new_mag_app);
  // printf("new gamma: %6.2f deg\n", Rad2Deg(*new_gamma_sail));   
  double delta = *new_gamma_sail - old_gamma_sail;
  // printf("old gamma: %6.2f deg\n", Rad2Deg(old_gamma_sail));   
  // printf("delta: %6.2f deg %g\n", Rad2Deg(delta), Sign(delta));   
  
  if (*maneuver_type == kJibe)
    *delta_gamma_sail = delta - 2 * M_PI * Sign(delta);
  else
    *delta_gamma_sail = delta; 
}
