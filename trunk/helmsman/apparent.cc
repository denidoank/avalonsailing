// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/apparent.h"

#include "common/normalize.h"

void Apparent(double angle_true, double mag_true,
              double angle_boat, double mag_boat,
              double phi_z,
              double* angle_app, double* mag_app) {
  // apparent wind in global frame
  Polar apparent = Polar(angle_true, mag_true) - Polar(angle_boat, mag_boat);
  *mag_app = apparent.Mag();
  *angle_app = SymmetricRad(apparent.AngleRad() - phi_z);
}

void ApparentPolar(const Polar& true_wind,
                   const Polar& boat_speed,
                   double phi_z,  
                   Polar* apparent_wind_on_boat) {
  // apparent wind in global frame
  Polar apparent_global = true_wind - boat_speed;
  *apparent_wind_on_boat =
      Polar(SymmetricRad(apparent_global.AngleRad() - phi_z),
            apparent_global.Mag());
}

void TruePolar(const Polar& apparent_wind_on_boat,
               const Polar& boat_speed,
               double phi_z,
               Polar* true_wind) {
  double alpha_app_global = phi_z + apparent_wind_on_boat.AngleRad();
  *true_wind = Polar(alpha_app_global, apparent_wind_on_boat.Mag()) + boat_speed;
}
