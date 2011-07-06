// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/apparent.h"

// Actually an approximation is used here assumong that the motion direction of
// the boat is identical with its orientation. The drift of a few degrees is
// neglegted.

#include "common/normalize.h"

void Apparent(double angle_true, double mag_true,
              double angle_boat, double mag_boat,
              double* angle_app, double* mag_app) {
  // apparent wind in global frame
  Polar apparent = Polar(angle_true, mag_true) - Polar(angle_boat, mag_boat);
  *mag_app = apparent.Mag();
  *angle_app = SymmetricRad(apparent.AngleRad() - angle_boat);
}

void ApparentPolar(const Polar& true_wind,
                   const Polar& boat_speed,
                   Polar* apparent_wind_on_boat) {
  // apparent wind in global frame
  Polar apparent_global = true_wind - boat_speed;
  *apparent_wind_on_boat =
      Polar(SymmetricRad(apparent_global.AngleRad() - boat_speed.AngleRad()),
            apparent_global.Mag());
}

void TruePolar(const Polar& apparent_wind_on_boat,
               const Polar& boat_speed,
               Polar* true_wind) {
  Polar true_wind_on_boat = apparent_wind_on_boat + boat_speed;
  *true_wind =
      Polar(SymmetricRad(true_wind_on_boat.AngleRad() + boat_speed.AngleRad()),
            true_wind_on_boat.Mag());
}
