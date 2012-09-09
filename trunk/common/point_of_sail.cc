// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

#include "common/point_of_sail.h"


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
// TODO: 4 limits as array.
double PointOfSail::SailableHeading(
    double alpha_star,
    double alpha_true,
    double alpha_app,
    double mag_app,
    double phi_z,
    double previous_output,  // previous output direction, needed to implement hysteresis
    SectorT* sector,      // sector codes for state handling and maneuver
    double* target) {
  double limit1t = SymmetricRad(alpha_true - M_PI - TackZoneRad());
  double limit2t = SymmetricRad(alpha_true - M_PI + TackZoneRad());
  double limit3t = SymmetricRad(alpha_true - JibeZoneHalfWidthRad());
  double limit4t = SymmetricRad(alpha_true + JibeZoneHalfWidthRad());
  if (debug) {
    fprintf(stderr, "limits true : % 5.2lf % 5.2lf % 5.2lf % 5.2lf\n",
            limit1t, limit2t, limit3t, limit4t);
  }

  const double decay = Deg2Rad(0.2) * 0.1;  // 0.2 degree per second (very slow)

  double limit1 = limit1t;
  double limit2 = limit2t;
  double limit3 = limit3t;
  double limit4 = limit4t;
  double limit1a;
  double limit2a;
  double limit3a;
  double limit4a;
  if (mag_app > 0.5) {
    // For the apparent wind the tack zone is smaller and the jibe zone is bigger.
    // For the Jibe zone we do not take this into account because it will not
    // have much of a negative effect, but would reduce the sailable angles
    // a lot.
    const double kAppOffset = Deg2Rad(5);
    limit1a = SymmetricRad(phi_z + alpha_app - M_PI - TackZoneRad() + kAppOffset);
    limit2a = SymmetricRad(phi_z + alpha_app - M_PI + TackZoneRad() - kAppOffset);
    limit3a = SymmetricRad(phi_z + alpha_app - JibeZoneHalfWidthRad());
    limit4a = SymmetricRad(phi_z + alpha_app + JibeZoneHalfWidthRad());
    if (debug) {
      fprintf(stderr, "limits app  : % 5.2lf % 5.2lf % 5.2lf % 5.2lf\n",
              limit1a, limit2a, limit3a, limit4a);
    }
    double delta1 = DeltaOldNewRad(limit1t, limit1a);  // neg
    double delta2 = DeltaOldNewRad(limit2t, limit2a);  // pos
    double delta3 = DeltaOldNewRad(limit3t, limit3a);  // neg
    double delta4 = DeltaOldNewRad(limit4t, limit4a);  // pos

    limit1 = SymmetricRad(limit1t + NegativeFilterOffset(delta1, decay, &buffer1_));
    limit2 = SymmetricRad(limit2t + PositiveFilterOffset(delta2, decay, &buffer2_));
    limit3 = SymmetricRad(limit3t + NegativeFilterOffset(delta3, decay, &buffer3_));
    limit4 = SymmetricRad(limit4t + PositiveFilterOffset(delta4, decay, &buffer4_));
  }

  if (debug) {
    fprintf(stderr, "limits mixed: % 5.2lf % 5.2lf % 5.2lf % 5.2lf\n",
            limit1, limit2, limit3, limit4);
  }

  // If true and apparent wind contradict each other, the apparent wind is taken.
  // For a quickly turning apparent wind this switch can fail and an alpha* could exist
  // that falls between 2 cases. So the decision later would fail.
  // The apparent wind is filtered with a smaller time constant, so it is the
  // more relevant limitation here.
  if (Less(limit3, limit2) || Less(limit1, limit4)) {
    limit1 = limit1a;
    limit2 = limit2a;
    limit3 = limit3a;
    limit4 = limit4a;
    if (debug) {
      fprintf(stderr, "lbroken,take: % 5.2lf % 5.2lf % 5.2lf % 5.2lf\n",
            limit1, limit2, limit3, limit4);
    }
  }

  // This keeps a small part of the previous output. The hysteresis is
  // good for minimizing the number of maneuvers but bad for the reaction
  // on quick wind turns.
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
    alpha_star_limited =
        BestSailableFallback(alpha_star, limit1, limit2, limit3, limit4, sector, target);
  }

  if (debug) {
    fprintf(stderr, "limits:       % 5.2lf % 5.2lf % 5.2lf % 5.2lf alpha*: %lf sector %d %c\n",
            limit1, limit2, limit3, limit4, alpha_star, int(*sector), left?'L':'R');
  }

  return alpha_star_limited;
}

void PointOfSail::Reset() {
  buffer1_ = 0;
  buffer2_ = 0;
  buffer3_ = 0;
  buffer4_ = 0;
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
    return FilterOffset(in, decay, prev);
  else
    return FilterOffset(0, decay, prev);
}

double NegativeFilterOffset(double in, double decay, double* prev) {
  if (in < 0)
    return FilterOffset(in, decay, prev);
  else
    return FilterOffset(0, decay, prev);
}

