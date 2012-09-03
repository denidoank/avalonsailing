// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

#include "common/polar_diagram.h"


#include <math.h>
#include <stdio.h>

#include "common/check.h"
#include "common/convert.h"
#include "common/delta_angle.h"
#include "common/normalize.h"

extern int debug;

namespace {
const double kTackZoneDeg = 50;        // degrees, a safe guess.
const double kJibeZoneDeg = 180 - 20;  // degrees
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

double JibeZoneHalfWidthRad() {
  return Deg2Rad(180 - kJibeZoneDeg);
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

namespace {
// paranoid fallback function
double BestSailableFallback(double alpha_star,
                            double limit1, double limit2,
                            double limit3, double limit4,
                            SectorT* sector, double* target) {
  if (fabs(alpha_star - limit1) < 0.1) {
    *sector = TackPort;
    *target = limit1;
    return limit1;
  }
  if (fabs(alpha_star - limit2) < 0.1) {
    *sector = TackStar;
    *target = limit2;
    return limit2;
  }
  if (fabs(alpha_star - limit3) < 0.1) {
    *sector = JibeStar;
    *target = limit3;
    return limit3;
  }
  // if (fabs(alpha_star - limit4) < 0.1)
  *sector = JibePort;
  *target = limit4;
  return limit4;
}

}  // namespace


// We used to make this decision on the slowly filtered true
// wind only. But if the wind turns quickly, then we got to
// react on that later in the decision process and that means
// that we have an unclear distribution of responsibilities.
// A sudden turn of the wind might require a maneuver. So it has
// to be detected early.
// We use the apparent wind (which is not as inert as the
// true wind) and phi_z to calculate the momentary true wind
// and react to wind gusts at the very start of the normal controller logic.
// Everything in radians here.
double SailableHeading(double alpha_star,
                       double alpha_true,
                       double alpha_app,
                       double mag_app,
                       double phi_z,
                       double previous_output,  // previous output direction, needed to implement hysteresis
                       SectorT* sector,      // sector codes for state handling and maneuver
                       double* target) {
  double limit1t = alpha_true - M_PI - TackZoneRad();
  double limit2t = alpha_true - M_PI + TackZoneRad();
  double limit3t = alpha_true - JibeZoneHalfWidthRad();
  double limit4t = alpha_true + JibeZoneHalfWidthRad();
  if (debug) {
    fprintf(stderr, "limits true: %lf %lf %lf %lf\n",
            limit1t, limit2t, limit3t, limit4t);
  }

  double limit1 = limit1t;
  double limit2 = limit2t;
  double limit3 = limit3t;
  double limit4 = limit4t;
  if (mag_app > 0.5) {
    // For the apparent wind the tack zone is smaller and the jibe zone is bigger.
    // For the Jibe zone we do not take this into account because it will not
    // have much of a negative effect, but would reduce the sailable angles
    // a lot.
    const double kAppOffset = Deg2Rad(10);
    const double limit1a = phi_z + alpha_app - M_PI - TackZoneRad() + kAppOffset;
    const double limit2a = phi_z + alpha_app - M_PI + TackZoneRad() - kAppOffset;
    const double limit3a = phi_z + alpha_app - JibeZoneHalfWidthRad();
    const double limit4a = phi_z + alpha_app + JibeZoneHalfWidthRad();
    if (debug) {
      fprintf(stderr, "limits app: %lf %lf %lf %lf\n",
              limit1a, limit2a, limit3a, limit4a);
    }
    limit1 = SymmetricRad(Minimum(limit1t, limit1a));
    limit2 = SymmetricRad(Maximum(limit2t, limit2a));
    limit3 = SymmetricRad(Minimum(limit3t, limit3a));
    limit4 = SymmetricRad(Maximum(limit4t, limit4a));
  }

  if (debug) {
    fprintf(stderr, "limits mixed: %lf %lf %lf %lf\n",
            limit1, limit2, limit3, limit4);
  }

  // This keeps a small part of the previous output.
  double hysteresis = DeltaOldNewRad(previous_output, alpha_star) * 0.3;
  bool left = false;

  /* Sector defintion
  1, 2: left and right in the tack zone
  3   : Reaching with sail on starboard
  4, 5: right and left in the jibe zone
  6   : Reaching with sail on portside
  */
  *target = 0;
  double alpha_star_limited = alpha_star;
  if (LessOrEqual(limit1, alpha_star) && Less(alpha_star, limit2)) {
    // Modify if in the non-sailable tack range.
    alpha_star_limited = NearerRad(alpha_star - hysteresis, limit1, limit2, &left);
    *sector = left ? TackPort : TackStar;
    *target = left ? limit1 : limit2;
  } else if (LessOrEqual(limit2, alpha_star) && Less(alpha_star, limit3)) {
    *sector = ReachStar;
  } else if (LessOrEqual(limit3, alpha_star) && Less(alpha_star, limit4)) {
    // Modify if in the non-sailable jibe range.
    alpha_star_limited = NearerRad(alpha_star - hysteresis, limit3, limit4, &left);
    *sector = left ? JibeStar : JibePort;
    *target = left ? limit3 : limit4;
  } else if (LessOrEqual(limit4, alpha_star) && Less(alpha_star, limit1)) {
    *sector = ReachPort;
  } else {
    // Theoretically the numerics could fail and an alpha* could exist
    // that falls between 2 cases. No such test cases could be found.
    fprintf(stderr, "Illegal sector range: limits: %lf %lf %lf %lf alpha*: %lf\n",
            limit1, limit2, limit3, limit4, alpha_star);
    alpha_star_limited =
        BestSailableFallback(alpha_star, limit1, limit2, limit3, limit4, sector, target);
  }

  if (debug) {
    fprintf(stderr, "limits: %lf %lf %lf %lf alpha*: %lf sector %d %c\n",
            limit1, limit2, limit3, limit4, alpha_star, int(*sector), left?'L':'R');
  }

  return alpha_star_limited;
}

