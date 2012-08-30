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
// TODO Remove the
void Planner::Init(const LatLon& lat_lon) {
  if (ufenau.In(lat_lon)) {
    fprintf(stderr, "Ufenau plan\n");
    plan_.Build(ufenau_plan);
  } else if (toulon.In(lat_lon)) {
    fprintf(stderr, "Mallorca plan\n");
    plan_.Build(mallorca_plan);
  } else if (sukku.In(lat_lon)) {
    fprintf(stderr, "sukku plan\n");
    plan_.Build(sukku_plan);
  } else if (kilchberg.In(lat_lon)) {
    fprintf(stderr, "kilchberg plan\n");
    plan_.Build(kilchberg_plan);
  } else if (thalwil.In(lat_lon)) {
    fprintf(stderr, "thalwil plan\n");
    plan_.Build(thalwil_plan);
  } else if (horgen.In(lat_lon)) {
    fprintf(stderr, "horgen plan\n");
    plan_.Build(horgen_plan);
  } else if (au.In(lat_lon)) {
    fprintf(stderr, "au plan\n");
    plan_.Build(au_plan);
  } else if (waedenswil.In(lat_lon)) {
    fprintf(stderr, "waedenswil plan\n");
    plan_.Build(waedenswil_plan);
  } else if (wollerau.In(lat_lon)) {
    fprintf(stderr, "wollerau plan\n");
    plan_.Build(wollerau_plan);
  } else if (brest.In(lat_lon)) {
    fprintf(stderr, "carribean plan\n");
    plan_.Build(caribbean_plan);
  } else {
    fprintf(stderr, "According to our GPS we are not on lake zuerich.\n");
    fprintf(stderr, "lat %8.6lg lon %8.6lg \n", lat_lon.lat, lat_lon.lon);
    if (brest.In(lat_lon)) {
      fprintf(stderr, "According to our GPS we are near Brest.\n");
      fprintf(stderr, "lat %8.6lg lon %8.6lg \n", lat_lon.lat, lat_lon.lon);
    } else {
      fprintf(stderr, "According to our GPS we are in the middle of our journey\n");
      fprintf(stderr, "lat %8.6lg lon %8.6lg\n", lat_lon.lat, lat_lon.lon);
    }
    fprintf(stderr, "Mallorca plan\n");
    plan_.Build(mallorca_plan);
    fprintf(stderr, "Built the Mallorca plan\n");
    // TODO for Atlantic change the default.
    //fprintf(stderr, "Caribbean plan\n");
    //plan_.Build(caribbean_plan);
    //fprintf(stderr, "Built the Caribbean plan\n");
  }
}

double Planner::ToDeg(double lat_deg, double lon_deg) {
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
    alpha_star_ = plan_.ToDeg(lat_deg, lon_deg);
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
