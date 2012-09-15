// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "skipper/planner.h"
#include <stdio.h>

#include "helmsman/normal_controller.h"
#include "lib/util/stopwatch.h"
#include "skipper/plans.h"


double NowSeconds() {
  return StopWatch::GetTimestampMicros()/1E6;
}

bool Planner::initialized_ = false;
TargetCircleCascade Planner::plan_;
double Planner::alpha_star_ = 225;
double Planner::last_turn_time_ = 0;


// We may start from
// kilchbarg thalwil horgen au waedenswil wollerau
// will identify the nearest and sail towards the
// target point in the middle of the lake where we
// start to sail 2 minute legs in all directions
// for a better polar diagram and general testing.
// At the target of the real journey we will also
// sail criss-cross.
void Planner::Init(const LatLon& lat_lon) {
  fprintf(stderr, "According to our GPS we are not on lake zuerich.\n");
  fprintf(stderr, "lat %8.6lg lon %8.6lg \n", lat_lon.lat, lat_lon.lon);
  fprintf(stderr, "Toulon plan\n");
  plan_.Build(toulon_plan);
  fprintf(stderr, "Built the Toulon plan\n");
  // TODO for Atlantic change the default.
  //fprintf(stderr, "Caribbean plan\n");
  //plan_.Build(caribbean_plan);
  //fprintf(stderr, "Built the Caribbean plan\n");
}

double Planner::ToDeg(double lat_deg, double lon_deg, TCStatus* tc_status) {
 // fprintf(stderr, "ToDeg %lg %lg\n", lat_deg, lon_deg);

  CHECK(!isnan(lat_deg));
  CHECK(!isnan(lon_deg));

  LatLon lat_lon(lat_deg, lon_deg);
  if (!initialized_) {
    Init(lat_lon);
    initialized_ = true;
  }
  if (plan_.TargetReached(lat_lon)) {
    fprintf(stderr, "Hurray! Target Reached!\n");
    if (NowSeconds() > last_turn_time_ + 120) { // TODO: Increase this time after lake tests!
      last_turn_time_ = NowSeconds();
      alpha_star_ = NormalizeDeg(alpha_star_ - 72);
      fprintf(stderr, "Turn left to %lf.", alpha_star_);
    }
  } else {
    alpha_star_ = plan_.ToDeg(lat_deg, lon_deg, tc_status);
    // fprintf(stderr, "Planner alpha_star %lf.\n", alpha_star_);
  }
  return alpha_star_;
}

bool Planner::TargetReached(const LatLon& lat_lon){
  return plan_.TargetReached(lat_lon);
}

bool Planner::Initialized() {
  return initialized_;
}

void Planner::Reset() {
  initialized_ = false;
}

void Planner::SimplePlan(double lat_deg, double lon_deg) {
  TargetCirclePoint cover_all = TargetCirclePoint(lat_deg, lon_deg, 1);
  TargetCirclePoint exact = TargetCirclePoint(lat_deg, lon_deg, 0.004);  // Target reached circle with 500m radius.
  TargetCirclePoint end_marker = TargetCirclePoint(0, 0, 0);
  TargetCirclePoint management_summary[3] = {exact, cover_all, end_marker};
  plan_.Build(management_summary);
}
