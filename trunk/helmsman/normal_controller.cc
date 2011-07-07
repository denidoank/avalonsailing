// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

//#include "common/boat.h"  // constants from simulation/boat.m

#include "helmsman/normal_controller.h"

#include <math.h>
#include "common/delta_angle.h"
#include "common/normalize.h"
#include "common/polar_diagram.h"
#include "helmsman/apparent.h"
#include "helmsman/new_gamma_sail.h"
#include "lib/fm/log.h"

// TODO(grundmann): Tack Overshoot.

NormalController::NormalController(RudderController* rudder_controller,
                                   SailController* sail_controller)
   : rudder_controller_(rudder_controller),
     sail_controller_(sail_controller),
     prev_alpha_star_limited_(0) {}

NormalController::~NormalController() {}

void NormalController::Entry(const ControllerInput& in,
                             const FilteredMeasurements& filtered){
  // Make sure we get the right idea of the direction change when we come
  // from another state.
  prev_alpha_star_limited_ = SymmetricRad(filtered.phi_z_boat); 
  ref_.SetReferenceValues(prev_alpha_star_limited_, in.drives.gamma_sail_rad);
}     

void NormalController::Run(const ControllerInput& in,
                           const FilteredMeasurements& filtered,
                           ControllerOutput* out) {
  FM_LOG_DEBUG("Ref: %6.4f", Rad2Deg(in.alpha_star_rad));
  FM_LOG_DEBUG("Actuals: True %6.4f deg %6.4f m/s",
               Rad2Deg(filtered.alpha_true), filtered.mag_true);
  FM_LOG_DEBUG("Actuals: Boat %6.4f deg %6.4f m/s",
               Rad2Deg(filtered.phi_z_boat), filtered.mag_boat);
  FM_LOG_DEBUG("Actuals: App  %6.4f deg %6.4f m/s",
               Rad2Deg(filtered.angle_app),  filtered.mag_app);

  double phi_star;
  double omega_star;
  double gamma_sail_star;
  ReferenceValueSwitch(SymmetricRad(in.alpha_star_rad),
                       SymmetricRad(filtered.alpha_true), filtered.mag_true,
                       SymmetricRad(filtered.phi_z_boat), filtered.mag_boat,
                       SymmetricRad(filtered.angle_app),  filtered.mag_app,
                       in.drives.gamma_sail_rad,
                       &phi_star,
                       &omega_star,
                       &gamma_sail_star);
  FM_LOG_DEBUG("IntRef: %6.4f %6.4f %6.4f", Rad2Deg(phi_star),
               Rad2Deg(omega_star), Rad2Deg(gamma_sail_star));

  double gamma_rudder_star;
  rudder_controller_->Control(phi_star,
                              omega_star,
                              SymmetricRad(filtered.phi_z_boat),
                              filtered.omega_boat,
                              filtered.mag_boat,
                              &gamma_rudder_star);

  out->drives_reference.gamma_rudder_star_left_rad  = gamma_rudder_star;
  out->drives_reference.gamma_rudder_star_right_rad = gamma_rudder_star;
  out->drives_reference.gamma_sail_star_rad = gamma_sail_star;                           
  FM_LOG_DEBUG("Controls: %6.4f %6.4f",
               Rad2Deg(gamma_rudder_star), Rad2Deg(gamma_sail_star));
}

void NormalController::Exit() {}



void NormalController::ReferenceValueSwitch(double alpha_star,
                                            double alpha_true, double mag_true,
                                            double phi_z_boat, double mag_boat,
                                            double angle_app,  double mag_app,
                                            double old_gamma_sail,
                                            double* phi_z_star,
                                            double* omega_z_star,
                                            double* gamma_sail_star) {

  // Stay in sailable zone
  double angle_from_wind = SymmetricRad(alpha_star - alpha_true - M_PI);
  double alpha_star_limited = alpha_star;
  if (angle_from_wind < 0 && angle_from_wind > -TackZoneRad())
    alpha_star_limited = SymmetricRad(alpha_true - M_PI - TackZoneRad());  
  else if (angle_from_wind >= 0 && angle_from_wind < TackZoneRad())
    alpha_star_limited = SymmetricRad(alpha_true - M_PI + TackZoneRad());

  if (!ref_.RunningPlan() &&
      fabs(DeltaOldNewRad(alpha_star_limited, prev_alpha_star_limited_)) >
          Deg2Rad(20)) {
    // The heading just jumped by a lot, need a new plan.
    double new_gamma_sail;
    double delta_gamma_sail;
    ManeuverType maneuver_type = FindManeuverType(prev_alpha_star_limited_,
                                                  alpha_star_limited,
                                                  alpha_true);
    NewGammaSailWithOldGammaSail(alpha_true, mag_true,
                 phi_z_boat, mag_boat,
                 alpha_star_limited,
                 old_gamma_sail,
                 maneuver_type,
                 sail_controller_,
                 &new_gamma_sail,
                 &delta_gamma_sail);
    if (true) {
      ref_.SetReferenceValues(prev_alpha_star_limited_, old_gamma_sail);
      ref_.NewPlan(alpha_star_limited,
                   delta_gamma_sail,
                   mag_boat);
    }               
    prev_alpha_star_limited_ = alpha_star_limited;
    ref_.GetReferenceValues(phi_z_star, omega_z_star, gamma_sail_star);  
  } else if (ref_.RunningPlan()) {
    ref_.GetReferenceValues(phi_z_star, omega_z_star, gamma_sail_star);  
    // We don't accept new reference values here, so we also don't follow up
    // with the previous one.
  } else {
    // Normal case, minor changes of the desired heading.
    *phi_z_star = alpha_star_limited;
    *omega_z_star = 0;
    *gamma_sail_star = 
        sail_controller_->BestStabilizedGammaSail(angle_app, mag_app);
    prev_alpha_star_limited_ = alpha_star_limited;
  }
}

bool NormalController::Tacking() {
  return ref_.RunningPlan();
}
