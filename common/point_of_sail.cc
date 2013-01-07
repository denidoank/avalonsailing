// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

#include "common/point_of_sail.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>

#include "common/angle.h"
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
// react on that with the AntiWindGust method.
// Everything in radians here.
// If alpha_star and alpha_true are rate limited then the sector
// output doesn't jump.
Angle PointOfSail::SailableHeading(
    Angle alpha_star,       // rate limited alpha star
    Angle alpha_true,       // true wind vector, slowly filtered
    Angle previous_output,  // previous output direction, needed to implement hysteresis
    SectorT* sector,        // sector codes for state handling and maneuver
    Angle* target) {
  Angle limit1 = alpha_true.opposite() - TackZone();
  Angle limit2 = alpha_true.opposite() + TackZone();
  Angle limit3 = alpha_true - JibeZoneHalfWidth();
  Angle limit4 = alpha_true + JibeZoneHalfWidth();
  if (debug) {
    fprintf(stderr, "limits true : % 5.2lf % 5.2lf % 5.2lf % 5.2lf degrees\n",
            limit1.deg(), limit2.deg(), limit3.deg(), limit4.deg());
  }

  // This keeps a small part of the previous output, later used in the hysteresis
  // calculation. The hysteresis is good for minimizing the number of maneuvers.
  Angle hysteresis_tack = (alpha_star - previous_output) * 0.1; // about 5 degrees
  Angle hysteresis_jibe = (alpha_star - previous_output) * 0.3;
  bool left = false;

  /* Sector defintion
  1, 2: left and right in the tack zone
  3   : Reaching with sail on starboard
  4, 5: right and left in the jibe zone
  6   : Reaching with sail on portside
  */
  *target = 0;
  Angle alpha_star_limited = alpha_star;

  if (limit1 <= alpha_star && alpha_star < limit2) {
    // Modify if in the non-sailable tack range.
    alpha_star_limited = (alpha_star - hysteresis_tack).nearest(limit1, limit2, &left);
    *sector = left ? TackPort : TackStar;
    *target = left ? limit1 : limit2;
  } else if (limit2 <= alpha_star && alpha_star < limit3) {
    *sector = ReachStar;
  } else if ((limit3 <= alpha_star) && (alpha_star < limit4)) {
    // Modify if in the non-sailable jibe range.
    alpha_star_limited = (alpha_star - hysteresis_jibe).nearest(limit3, limit4, &left);
    *sector = left ? JibeStar : JibePort;
    *target = left ? limit3 : limit4;
  } else if ((limit4 <= alpha_star) && (alpha_star < limit1)) {
    *sector = ReachPort;
  } else {
    fprintf(stderr, "Illegal sector range: limits: %lf %lf %lf %lf alpha*: %lf degrees\n",
            limit1.deg(), limit2.deg(), limit3.deg(), limit4.deg(), alpha_star.deg());
    limit1.print("limit1");
    limit2.print("limit2");
    limit3.print("limit3");
    limit4.print("limit4");
    alpha_star.print("alpha_star");
    CHECK(0);
  }

  if (debug) {
    fprintf(stderr, "limits:       % 5.2lf % 5.2lf % 5.2lf % 5.2lf alpha*: %lf degrees, sector %d %c\n",
            limit1.deg(), limit2.deg(), limit3.deg(), limit4.deg(), alpha_star.deg(), int(*sector), left?'L':'R');
  }

  return alpha_star_limited;
}

// We use the apparent wind (which is not as inert as the
// true wind) and react to wind gusts at the very start
// of the normal controller logic.
// If the sector is stable, we can calculate a bearing correction in response to fast wind
// turns.
Angle PointOfSail::AntiWindGust(SectorT sector,      // sector codes
                                Angle alpha_app,
                                double mag_app_m_s) {
  // If the apparent wind has direct influence on the desired heading then
  // an instable system is created. We observed such oscillations during our
  // lake tests: their exact mechanism is not clear. We apply an asymmetric
  // change rate limitation, i.e. if we notice that we went too much into the wind then
  // we fall off quickly and return very slowly only. Just like a real helmsman.
  const Angle decay = deg(0.2 * 0.1);  // 0.2 degree per second (very slow)

  Angle correction;
  if (mag_app_m_s > 0.5) {
    // For the apparent wind the tack zone is smaller and the jibe zone is bigger.
    // For the Jibe zone we do not take this into account because it will not
    // have much of a negative effect, but would reduce the sailable angles
    // a lot.
    const Angle kAppOffset = deg(12);  // TODO: 10 degrees seems to be better.
    // Bigger than 0, if we are too close and have to fall off left.
    Angle delta1 = rad(TackZoneRad()).opposite() - kAppOffset - alpha_app;
    // Bigger than 0, if we shall fall off right.
    Angle delta2 = -rad(TackZoneRad()).opposite() + kAppOffset - alpha_app;

    // What do these deltas mean? It means that the normal control has failed or
    // that the wind turned quickly.
    // If delta1 is positive then the current phi_z (precisely the phi_z averaged during the
    // measuring period of the most recent available apparent wind) should be changed by
    // (-delta1). By doing that the boat would turn in such a way that it steers at the limit of the
    // tack zone.
    // If delta1 is negative it can be ignored.
    switch (sector) {
      case TackPort:
      case ReachPort:
        buffer2_ = 0;
        correction = -PositiveFilterOffset(delta1, decay, &buffer1_);
        if (debug && correction.negative()) {
          fprintf(stderr, "corr1 FALL OFF LEFT: % 5.2lf \n",  correction.deg());
        }
        break;
      case TackStar:
      case ReachStar:
        buffer1_ = 0;
        correction = PositiveFilterOffset(-delta2, decay, &buffer2_);
        if (debug && correction > 0) {
          fprintf(stderr, "corr2 FALL OFF RIGHT: % 5.2lf \n",  correction.deg());
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

Angle FilterOffset(Angle in, Angle decay, Angle* prev) {
  if (in.sign() != 0 && in.sign() != prev->sign()) {
   *prev = in;
    return in;
  }
  if (in.positive() || prev->positive()) {
    if (prev->positive())
      *prev -= decay;
    if (in > *prev)
      *prev = in;
  }
  if (in.negative() || prev->negative()) {
    if (prev->negative())
      *prev += decay;
    if (in < *prev)
      *prev = in;
  }
  return *prev;
}

Angle PositiveFilterOffset(Angle in, Angle decay, Angle* prev) {
  if (in.positive()) {
    // clip the correction at 45 degrees.
    return FilterOffset(std::min(in, deg(45)), decay, prev);
  } else {
    return FilterOffset(deg(0), decay, prev);
  }
}
