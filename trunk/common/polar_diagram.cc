// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
#include <math.h>

#include "common/check.h"
#include "common/convert.h"
#include "common/normalize.h"

const double kTackZone = 50;        // degrees, a safe guess.
const double kJibeZone = 180 - 15;  // degrees, might be 170 at storm

double Speed(double angle, double wind_speed);

// In reality the polar diagram of the boat speeds (and the zone angles) depend
// on the wind speed. We could not measure them so far, so we define a flexible
// interface and use an approximation.
// If the wind speed is unknown, give 10.0 for the wind speed to get the
// relative speed of the boat for the given angle.
// See http://www.oppedijk.com/zeilen/create-polar-diagram or
// http://www.scribd.com/doc/13811789/Segeln-gegen-den-Wind

void ReadPolarDiagram(double angle_true_wind,
                      double wind_speed,
                      bool* dead_zone_tack,
                      bool* dead_zone_jibe,
                      double* boat_speed) {
  CHECK_GE(wind_speed, 0);
  angle_true_wind = SymmetricAngle(angle_true_wind);  // convert to (-180, 180]
  CHECK_LE(angle_true_wind, 180);
  CHECK_GE(angle_true_wind, -180);

  angle_true_wind = fabs(angle_true_wind);
  *dead_zone_tack = angle_true_wind < kTackZone;
  *dead_zone_jibe = angle_true_wind > kJibeZone;

  if (*dead_zone_tack) {
    const double v_0 = Speed(kTackZone, wind_speed) *
                       cos(Deg2Rad(kTackZone));
    *boat_speed = v_0 / cos(Deg2Rad(angle_true_wind));
  } else if (*dead_zone_jibe) {
    const double v_180 = Speed(kJibeZone, wind_speed) *
                         cos(Deg2Rad(kJibeZone));
    *boat_speed = v_180 / cos(Deg2Rad(angle_true_wind));
  } else {
    *boat_speed = Speed(angle_true_wind, wind_speed);
  }
}

// Boat speed as a function of the wind speed and the angle between the inverted
// wind vector and the boat heading direction, i.e. the angle is 0 degree if the
// boat is pointing into the direction where the wind comes FROM and the angle
// is 90 if the true wind is coming exactly from the side.
// This approximation was derived from 3 examples of polar diagrams found in the
// literature for wind speeds around 10knots. The angle range is limited to
// 45 - 180 degree.

double Speed(double angle, double wind_speed) {
  CHECK_GE(angle, kTackZone);
  CHECK_LE(angle, kJibeZone);
  // We might adapt this for different wind speeds later.
  // This approximation was done for WindSpeed = 10 kts;
  const double K0 = -0.9844053104;
  const double K1 =  3.3123159119;
  const double K2 = -2.3354225154;
  const double K3 =  0.7061562329;
  const double K4 = -0.0797837181;

  double a = Deg2Rad(angle);
  double a2 = a*a;
  double a3 = a2*a;
  double a4 = a3*a;

  double relative_speed  = K4 * a4 + K3 * a3 + K2 * a2 + K1 * a + K0;
  CHECK_LT(relative_speed, 1);
  // The boat speed is not proportional to the wind speed in reality.
  return relative_speed * wind_speed;
}

double TackZone() {
  return kTackZone;
}

double JibeZone() {
  return kJibeZone;
}

