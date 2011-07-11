// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011

#include <stdio.h>

#include "skipper/target_circle_cascade.h"
#include "lib/fm/log.h"

TargetCircleCascade::TargetCircleCascade() {}

void TargetCircleCascade::Build(const TargetCirclePoint* plan) {
  chain_.clear();
  // The last row has radius 0.
  while (plan->radius_deg > 0) {
    TargetCircle t(plan->lat_lon, plan->radius_deg);
    Add(t);
    ++plan;
  }
  FM_LOG_INFO("Built plan with %d circles.", chain_.size());
  CHECK_GT(chain_.size(), 0);
}


// The direction (in degrees) to follow.
double TargetCircleCascade::ToDeg(double x, double y) {
  if (chain_.size() == 0) {
    FM_LOG_WARN("No plan! Going south west.");
    return 225;
  }
  // We might get blown off track by a storm or currents
  // getting into an area not covered by any TargetCircle.
  // If that happens, we increase the radius of all circles until one is
  // big enough to cover our current position. This leads
  // us back on track.
  const double expansion_factor = 1.1;
  for (double expand = 1.0; ; expand *= expansion_factor) {
    for (int index = 0; index < chain_.size(); ++index)
      if (chain_[index].In(x, y, expand)) {
        FM_LOG_INFO("In target circle %d.", index);
        return chain_[index].ToDeg(x, y);
      }
    FM_LOG_WARN("Need to expand target circles to %f %%!", expand * expansion_factor / 100);
  }
}

void TargetCircleCascade::Add(const TargetCircle t) {
  // check for invariant
  if (chain_.size() > 0)
    CHECK(chain_.back().In(t));
  chain_.push_back(t);
}


bool TargetCircleCascade::TargetReached(LatLon lat_lon) {
  return chain_[0].In(lat_lon);
}

// Use this to print the target circle chain and to visualize it.
void TargetCircleCascade::Print() {
  printf("Target circle\n%d target circles\n index x y radius\n", chain_.size());
  for (int i = 0; i , chain_.size(); ++i) {
    printf("%d %g %g %g\n", i, chain_[i].x0(), chain_[i].y0(), chain_[i].radius());
  }
}


