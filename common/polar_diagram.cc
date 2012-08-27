// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
#include <math.h>

#include "common/check.h"
#include "common/convert.h"
#include "common/delta_angle.h"
#include "common/normalize.h"

namespace {
const double kTackZoneDeg = 50;        // degrees, a safe guess.
const double kJibeZoneDeg = 180 - 15;  // degrees, might be 170 at storm
}  // namespace

double Speed(double angle_deg, double wind_speed);

// In reality the polar diagram of the boat speeds (and the zone angles) depend
// on the wind speed. We could not measure them so far, so we define a flexible
// interface and use an approximation.
// If the wind speed is unknown, give 10.0 for the wind speed to get the
// relative speed of the boat for the given angle.
// See http://www.oppedijk.com/zeilen/create-polar-diagram or
// http://www.scribd.com/doc/13811789/Segeln-gegen-den-Wind

void ReadPolarDiagram(double angle_true_wind_deg,
                      double wind_speed_m_s,
                      bool* dead_zone_tack,
                      bool* dead_zone_jibe,
                      double* boat_speed_m_s) {
  if (wind_speed_m_s < 0.01) {
    *dead_zone_tack = false;
    *dead_zone_jibe = false;
    *boat_speed_m_s = 0;
    return;
  }
  angle_true_wind_deg = fabs(SymmetricDeg(angle_true_wind_deg));  // convert to [-180, 180)
  *dead_zone_tack = angle_true_wind_deg < kTackZoneDeg;
  *dead_zone_jibe = angle_true_wind_deg > kJibeZoneDeg;

  if (*dead_zone_tack) {
    const double v_0 = Speed(kTackZoneDeg, wind_speed_m_s) *
                       cos(Deg2Rad(kTackZoneDeg));
    *boat_speed_m_s = v_0 / cos(Deg2Rad(angle_true_wind_deg));
  } else if (*dead_zone_jibe) {
    const double v_180 = Speed(kJibeZoneDeg, wind_speed_m_s) *
                         cos(Deg2Rad(kJibeZoneDeg));
    *boat_speed_m_s = v_180 / cos(Deg2Rad(angle_true_wind_deg));
  } else {
    *boat_speed_m_s = Speed(angle_true_wind_deg, wind_speed_m_s);
  }
}

// Boat speed as a function of the wind speed and the angle between the inverted
// wind vector and the boat heading direction, i.e. the angle is 0 degree if the
// boat is pointing into the direction where the wind comes FROM and the angle
// is 90 if the true wind is coming exactly from the side.
// This approximation was derived from 3 examples of polar diagrams found in the
// literature for wind speeds around 10knots. The angle range is limited to
// 45 - 180 degree.

double Speed(double angle_deg, double wind_speed) {
  CHECK_GE(angle_deg, kTackZoneDeg);
  CHECK_LE(angle_deg, kJibeZoneDeg);
  // We might adapt this for different wind speeds later.
  // This approximation was done for WindSpeed = 10 kts;
  const double K0 = -2.9839914453775590E-01;
  const double K1 =  1.2774937184785125E-02;
  const double K2 = -6.7907132882375607E-05;
  const double K3 =  7.9556974926732012E-08;

  // y = a + bx + cx^2  + dx^3
  // [zunzun.com]

  double a = angle_deg;
  double a2 = a*a;
  double a3 = a2*a;

  double relative_speed  = K3 * a3 + K2 * a2 + K1 * a + K0;
  CHECK_LT(relative_speed, 1);
  // The boat speed is not proportional to the wind speed in reality.
  if (wind_speed > 5) {
    wind_speed = 4 + sqrt(wind_speed - 4);
  }
  // and the boat hull has high resistance above 2.3m/s.
  // TODO(grundmann): Improve this once we have strong wind data.
  double boat_speed = relative_speed * wind_speed;
  if (boat_speed > 2.3) {
    boat_speed = 1.3 + sqrt(boat_speed - 1.3);
  }
  if (boat_speed > 2.6) {
    boat_speed = 2.6;
  }
  return boat_speed;
}


double TackZoneDeg() {
  return kTackZoneDeg;
}

double JibeZoneDeg() {
  return kJibeZoneDeg;
}

double TackZoneRad() {
  return Deg2Rad(kTackZoneDeg);
}

double JibeZoneRad() {
  return Deg2Rad(kJibeZoneDeg);
}

double BestSailableHeading(double alpha_star, double alpha_true) {
  // Stay in sailable zone
  double tack_zone_min = Reverse(alpha_true) - TackZoneRad();
  double tack_zone_max = Reverse(alpha_true) + TackZoneRad();
  // A small instable zone around the dead run shall be avoided. There are certain dangers here,
  // (death roll), but if they actually are important for Avalon is unclear.
  static const double jibe_zone = M_PI - JibeZoneRad();
  CHECK(jibe_zone > 0);
  double jibe_zone_min = alpha_true - jibe_zone;
  double jibe_zone_max = alpha_true + jibe_zone;
  double alpha_star_limited = alpha_star;

  // Modify if in the non-sailable range.
  if (DeltaOldNewRad(tack_zone_min, alpha_star) > 0 &&
      DeltaOldNewRad(tack_zone_max, alpha_star) < 0) {
    alpha_star_limited = NearerRad(alpha_star, tack_zone_min, tack_zone_max);
  }
  if (DeltaOldNewRad(jibe_zone_min, alpha_star) > 0 &&
      DeltaOldNewRad(jibe_zone_max, alpha_star) < 0) {
    alpha_star_limited = NearerRad(alpha_star, jibe_zone_min, jibe_zone_max);
  }
  return SymmetricRad(alpha_star_limited);
}

// with hysteresis
double BestStableSailableHeading(double alpha_star, double alpha_true, double previous_output) {
  // Stay in sailable zone
  double tack_zone_min = Reverse(alpha_true) - TackZoneRad();
  double tack_zone_max = Reverse(alpha_true) + TackZoneRad();
  // A small instable zone around the dead run shall be avoided. There are certain dangers here,
  // (death roll), but if they actually are important for Avalon is unclear.
  static const double jibe_zone = M_PI - JibeZoneRad();
  CHECK(jibe_zone > 0);
  double jibe_zone_min = alpha_true - jibe_zone;
  double jibe_zone_max = alpha_true + jibe_zone;
  double alpha_star_limited = alpha_star;

  // This keeps a small part of the previous output.
  double hysteresis = DeltaOldNewRad(previous_output, alpha_star) * 0.3;

  // Modify if in the non-sailable range.
  if (DeltaOldNewRad(tack_zone_min, alpha_star) > 0 &&
      DeltaOldNewRad(tack_zone_max, alpha_star) < 0) {
    alpha_star_limited = NearerRad(alpha_star - hysteresis, tack_zone_min, tack_zone_max);
  }
  if (DeltaOldNewRad(jibe_zone_min, alpha_star) > 0 &&
      DeltaOldNewRad(jibe_zone_max, alpha_star) < 0) {
    alpha_star_limited = NearerRad(alpha_star - hysteresis, jibe_zone_min, jibe_zone_max);
  }
  return SymmetricRad(alpha_star_limited);
}






double BestSailableHeadingDeg(double alpha_star_deg, double alpha_true_deg){
  return Rad2Deg(BestSailableHeading(Deg2Rad(alpha_star_deg),
                                     Deg2Rad(alpha_true_deg)));
}

