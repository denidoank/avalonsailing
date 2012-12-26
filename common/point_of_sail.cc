// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

#include "common/point_of_sail.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>

#include "common/check.h"
#include "common/convert.h"
#include "common/delta_angle.h"
#include "common/normalize.h"
#include "common/polar_diagram.h"
#include "common/sign.h"

extern int debug;

PointOfSail::PointOfSail() {
  Reset();
}

// We make the decision about the point of sail on the slowly
// filtered true wind only. If the wind turns quickly, then we
// react on that early.
// Everything in radians here.
// TODO: 4 limits as array.
double PointOfSail::SailableHeading(
    double alpha_star,
    double alpha_true,
    double previous_output,  // previous output direction, needed to implement hysteresis
    SectorT* sector,         // sector codes for state handling and maneuver
    double* target) {
  double limit1 = SymmetricRad(alpha_true - M_PI - TackZoneRad());
  double limit2 = SymmetricRad(alpha_true - M_PI + TackZoneRad());
  double limit3 = SymmetricRad(alpha_true - JibeZoneHalfWidthRad());
  double limit4 = SymmetricRad(alpha_true + JibeZoneHalfWidthRad());
  if (debug) {
    fprintf(stderr, "limits true : % 5.2lf % 5.2lf % 5.2lf % 5.2lf\n",
            limit1, limit2, limit3, limit4);
  }


  // This keeps a small part of the previous output, later used in the hysteresis
  // calculation. The hysteresis is good for minimizing the number of maneuvers
  // but bad for the reaction on quick wind turns.
  double hysteresis_tack = DeltaOldNewRad(previous_output, alpha_star) * 0.1; // about 5 degrees
  double hysteresis_jibe = DeltaOldNewRad(previous_output, alpha_star) * 0.3;
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
    alpha_star_limited = NearerRad(alpha_star - hysteresis_tack, limit1, limit2, &left);
    *sector = left ? TackPort : TackStar;
    *target = left ? limit1 : limit2;
  } else if (LessOrEqual(limit2, alpha_star) && Less(alpha_star, limit3)) {
    *sector = ReachStar;
  } else if (LessOrEqual(limit3, alpha_star) && Less(alpha_star, limit4)) {
    // Modify if in the non-sailable jibe range.
    alpha_star_limited = NearerRad(alpha_star - hysteresis_jibe, limit3, limit4, &left);
    *sector = left ? JibeStar : JibePort;
    *target = left ? limit3 : limit4;
  } else if (LessOrEqual(limit4, alpha_star) && Less(alpha_star, limit1)) {
    *sector = ReachPort;
  } else {
    fprintf(stderr, "Illegal sector range: limits: %lf %lf %lf %lf alpha*: %lf\n",
            limit1, limit2, limit3, limit4, alpha_star);
    CHECK(0);
  }

  if (debug) {
    fprintf(stderr, "limits:       % 5.2lf % 5.2lf % 5.2lf % 5.2lf alpha*: %lf sector %d %c\n",
            limit1, limit2, limit3, limit4, alpha_star, int(*sector), left?'L':'R');
  }

  return alpha_star_limited;
}

// We use the apparent wind (which is not as inert as the
// true wind) and react to wind gusts at the very start
// of the normal controller logic.
// If the sector is stable, we can calculate a bearing correction in response to fast wind
// turns.
double PointOfSail::AntiWindGust(SectorT sector,      // sector codes
                                 double alpha_app,
                                 double mag_app) {
  const double decay = Deg2Rad(0.2) * 0.1;  // 0.2 degree per second (very slow)

  double  correction = 0;
  if (mag_app > 0.5) {
    // For the apparent wind the tack zone is smaller and the jibe zone is bigger.
    // For the Jibe zone we do not take this into account because it will not
    // have much of a negative effect, but would reduce the sailable angles
    // a lot.
    const double kAppOffset = Deg2Rad(5);
    double delta1 = SymmetricRad(-M_PI + TackZoneRad() - kAppOffset - alpha_app);  // negative, if we are too close and have to fall off right.
    double delta2 = SymmetricRad( M_PI - TackZoneRad() + kAppOffset - alpha_app);  // pos.
    if (debug) {
      fprintf(stderr, "app %lf  D1 %lf \ndeltas      : % 5.2lf % 5.2lf\n",
              alpha_app, M_PI - TackZoneRad() + kAppOffset, delta1, delta2);
    }
    // What do these deltas mean? It means that the normal control has failed or
    // that the wind turned quickly.
    // If delta1 is negative then the current phi_z (precisely the phi_z averaged during the
    // measuring period of the most recent available apparent wind) should be changed by
    // (-delta1). By doing that the boat would turn in such a way that it steers at the limit of the
    // tack or jibe zone.
    // If delta1 is positive it can be ignored.
    switch (sector) {
      case TackPort:
      case ReachPort:
        buffer2_ = 0;
        correction = -PositiveFilterOffset(delta1, decay, &buffer1_);
        if (debug && correction < 0) {
          fprintf(stderr, "corr1 FALL OFF LEFT: % 5.2lf \n",  correction);
        }
        break;
      case TackStar:
      case ReachStar:
        buffer1_ = 0;
        correction = PositiveFilterOffset(-delta2, decay, &buffer2_);
        if (debug && correction > 0) {
          fprintf(stderr, "corr2 FALL OFF RIGHT: % 5.2lf \n",  correction);
        }
        break;
      case JibeStar:
      case JibePort:
        correction = 0;
        buffer1_ = 0;
        buffer2_ = 0;
        break;
    }

  }
  return correction;
}


void PointOfSail::Reset() {
  buffer1_ = 0;
  buffer2_ = 0;
}

double FilterOffset(double in, double decay, double* prev) {
  if (Sign(in) != 0 && Sign(in) != Sign(*prev)) {
    *prev = in;
    return in;
  }
  if (in > 0 || *prev > 0) {
    if (*prev > 0)
      *prev -= decay;
    if (in > *prev)
      *prev = in;
  }
  if (in < 0 || *prev < 0) {
    if (*prev < 0)
      *prev += decay;
    if (in < *prev)
      *prev = in;
  }
  return *prev;
}

double PositiveFilterOffset(double in, double decay, double* prev) {
  if (in > 0)
    // clip the correction at 45 degrees.
    return FilterOffset(std::min(in, Deg2Rad(45)), decay, prev);
  else
    return FilterOffset(0, decay, prev);
}

double NegativeFilterOffset(double in, double decay, double* prev) {
  if (in < 0)
    return FilterOffset(in, decay, prev);
  else
    return FilterOffset(0, decay, prev);
}

