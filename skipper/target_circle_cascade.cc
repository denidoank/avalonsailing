// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011

#include <stdio.h>

#include "skipper/target_circle_cascade.h"


static const double kDefaultDirection = 225;  // SouthWest as an approximation of the whole journey.
extern int debug;


TargetCircleCascade::TargetCircleCascade() {}

void TargetCircleCascade::Build(const TargetCirclePoint* plan) {
  chain_.clear();
  // The last row has radius 0.
  while (plan->radius_deg > 0) {
    TargetCircle t(plan->lat_lon, plan->radius_deg);
    Add(t);
    ++plan;
  }
  if (debug) fprintf(stderr, "Built plan with %d circles.\n", (int)chain_.size());
  CHECK_GT(chain_.size(), 0);
}

// The direction (in degrees) to follow.
double TargetCircleCascade::ToDeg(double x, double y, TCStatus* tc_status) {
  if (chain_.size() == 0) {
    fprintf(stderr, "No plan! Going south west.\n");
    if (tc_status) {
      tc_status->target_lat_deg = 0;
      tc_status->target_lon_deg = 0;
      tc_status->index = 0;
    }
    return kDefaultDirection;
  }
  // We might get blown off track by a storm or currents
  // getting into an area not covered by any TargetCircle.
  // If that happens, we increase the radius of all circles until one is
  // big enough to cover our current position. This leads
  // us back on track.
  double min_distance = 1E9;
  int best_tc = 0;
  for (int i = 0; i < (int)chain_.size(); ++i) {
    // if (debug) fprintf(stderr, "i : %d, expand: %lf\n", i, expand);
    if (chain_[i].In(x, y, 1.0)) {
      // if (debug) fprintf(stderr, "In target circle %d, dir %lf deg.\n", i, chain_[i].ToDeg(x, y));
      if (tc_status) {
        tc_status->target_lat_deg = chain_[i].x0();
        tc_status->target_lon_deg = chain_[i].y0();
        tc_status->index = i;
      }
      return chain_[i].ToDeg(x, y);
    }
    if (chain_[i].Distance(x, y) < min_distance) {
      min_distance = chain_[i].Distance(x, y);
      best_tc = i;
    }
  }
  if (debug) {
    fprintf(stderr, "Needed to expand target circles!\n");
  }
  // if (debug) fprintf(stderr, "In target circle %d, dir %lf deg.\n", i, chain_[i].ToDeg(x, y));
  if (tc_status) {
    tc_status->target_lat_deg = chain_[best_tc].x0();
    tc_status->target_lon_deg = chain_[best_tc].y0();
    tc_status->index = best_tc;
  }
  return chain_[best_tc].ToDeg(x, y);
}

void TargetCircleCascade::Add(const TargetCircle t) {
  // check for invariant
  if (chain_.size() > 0)
    CHECK(chain_.back().In(t));
  CHECK(t.radius() > 0);
  chain_.push_back(t);
}


bool TargetCircleCascade::TargetReached(LatLon lat_lon) {
  return chain_[0].In(lat_lon);
}

// Use this to print the target circle chain and to visualize it.
void TargetCircleCascade::Print() {
  printf("Target circle\n%d target circles\n index x y radius\n", (int)chain_.size());
  for (int i = 0; i < (int)chain_.size(); ++i) {
    printf("%d %lf %lf %lf\n", i, chain_[i].x0(), chain_[i].y0(), chain_[i].radius());
  }
}
