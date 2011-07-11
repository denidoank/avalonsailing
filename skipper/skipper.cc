// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011


#include "common/unknown.h"
#include "common/polar_diagram.h"
#include "helmsman/normal_controller.h"
#include "lib/fm/log.h"
#include "skipper/planner.h"
#include "skipper/skipper.h"


void Skipper::Run(const SkipperInput& in,
                  const vector<AISInfo>& ais,
                  double* alpha_star_deg) {
  double alpha_planner = Planner::ToDeg(in.latitude_deg, in.longitude_deg);

  double feasible = 225;
  if (in.angle_true_deg != kUnknown && in.mag_true_kn > 1)
    feasible = BestSailableHeadingDeg(alpha_planner, in.angle_true_deg);  // TODO move to common
  if (fabs(alpha_planner - feasible) > 0.1)
      FM_LOG_INFO("Override %8.6g with %8.6g because it is not sailable.", alpha_planner, feasible);
  *alpha_star_deg = feasible;
}

void Skipper::Init(const SkipperInput& in) {
  Planner::Init(LatLon(in.latitude_deg, in.longitude_deg));
}

bool Skipper::TargetReached(const LatLon& lat_lon){
  return Planner::TargetReached(lat_lon);
}

