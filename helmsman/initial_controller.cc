// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/initial_controller.h"  

#include <stdio.h>
#include "lib/fm/log.h"
#include "common/polar_diagram.h"
#include "common/delta_angle.h"
#include "helmsman/sampling_period.h"  

InitialController::InitialController() {
  Reset();
}
  
void InitialController::InitialController::Reset() {
  phase_ = SLEEP;
  gamma_sign_ = 1; 
  count_ = 0;
}

void InitialController::Run(const ControllerInput& in,
                            const FilteredMeasurements& filtered,
                            ControllerOutput* out) {
  double gamma_sail = 0;
  double gamma_rudder = 0;
  out->Reset();
  if (!filtered.valid) {
    FM_LOG_INFO("Filter not ready");
    return;
  }
  if (!in.drives.homed_sail ||
      !in.drives.homed_rudder_left ||
      !in.drives.homed_rudder_right) {
    FM_LOG_INFO("Drives not ready");
    return;
  }

  /* The sailable direction is not feasible if the boats heading is not safe.
   Plan B: Turtle and Kogge
  */
  double angle_sail = SymmetricRad(filtered.angle_sail);
  
  double angle_attack = in.drives.gamma_sail_rad - filtered.angle_app - M_PI;
  // They are correct
  printf("angle sail: %g angle_attack %g -> %g\n", angle_sail, angle_attack, SymmetricRad(-angle_attack +M_PI));
  if ( phase_ == SLEEP ) {
    gamma_sail = 0;
    gamma_rudder = 0;
    // angle_sail is neg. angle of attack (or wind in relatio to the sail).
    // It is measurable under all conditions.
    // wait 10s for all filters to settle
    if (++count_ > 10.0 / kSamplingPeriod) {
      count_ = 0;
      PhaseChoice(angle_sail); 
    }
  }  

  if (phase_ == TURTLE) {
    printf("TURTLE -1 Portside bow");
    gamma_rudder = Deg2Rad(+15) * (-gamma_sign_);
    // Fixed angle of attack
    gamma_sail = SymmetricRad(filtered.angle_app + Deg2Rad(10) * (-gamma_sign_) - M_PI); // for 180deg: +10
    if (fabs(angle_sail) > TackZoneRad()) {
      // We turned out of the forbidden zone.
      phase_ = KOGGE;
      printf("TURTLE to KOGGE %d", gamma_sign_);
    }
  }

  // phase KOGGE
  if (phase_ == KOGGE) {
    gamma_sail = gamma_sign_ * M_PI/2;
    
    double epsilon = DeltaRad(angle_sail, gamma_sign_ * M_PI/2);
    printf("%g\n", epsilon);
    // Bang-bang-controller for the heading
    if (epsilon < 0)
      gamma_rudder = Deg2Rad(10);
    else
      gamma_rudder = Deg2Rad(-10);
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


/*  Initial conditions and the phases to handle them. 

 

a)         \
            \
             V

              A
              H
              H
angle_sail = 150 deg, backwards, rudder at +15, until 
condition: SymmetricRad(angle_sail) > M_PI - TackZoneRad()

b)              /
               /
              V

              A
              H
              H
angle_sail = -150 deg, backwards, rudder at -15
condition: SymmetricRad(angle_sail) < TackZoneRad() - M_PI

c)            A
              H
              H

             A
            /
           /
angle_sail = 30 deg, sail away to the right *
angle sail > 0

d)         
              A
              H
              H

               A
                \
                 \
angle_sail = -30 deg, sail away to the left
            

angle sail 0-360          |  phase_ gamma_sign_
---------------------------------------------
0 - (180 - TackZoneDeg):   | KOGGE    -1
(180 - TackZoneDeg) - 180: | TURTLE   -1
180 ...  180+TackZoneDeg:  | TURTLE   +1
180+TackZoneDeg ... 360:   | KOGGE    +1
*/


void InitialController::PhaseChoice(double angle_sail) {
  if (angle_sail > M_PI - TackZoneRad()) {
    gamma_sign_ = -1;
    // sail away to starboard (right)
    phase_ = KOGGE;
    printf("KOGGE -1 Portside bow");
  } else if (angle_sail > TackZoneRad() - M_PI) {
    gamma_sign_ = +1;
    // sail away to portside (left)
    phase_ = KOGGE;
    printf("KOGGE +1 Starboard bow");
  } else {
    // not sailable
    phase_ = TURTLE;
    if (angle_sail < 0) {
      gamma_sign_ = -1;
      printf("TURTLE -1 Portside bow");
    } else {
      gamma_sign_ = +1;
      printf("TURTLE +1 Portside bow");
    }  
  }
}


