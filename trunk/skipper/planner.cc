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
double Planner:: last_turn_time_ = 0;


// We may start from
// kilchbarg thalwil horgen au waedenswil wollerau
// will identify the nearest and sail towards the
// target point in the middle of the lake where we
// start to sail 2 minute legs in all directions
// for a better polar diagram and general testing.
// At the target of the real journey we will also
// sail criss-cross.

void Planner::Init(const LatLon& lat_lon) {
  if (sukku.In(lat_lon)) {
    printf("sukku plan\n");
    fprintf(stderr, "sukku plan");
    plan_.Build(sukku_plan);
  } else if (kilchberg.In(lat_lon)) {
    printf("kilchberg plan\n");
    fprintf(stderr, "kilchberg plan");
    plan_.Build(kilchberg_plan);
  } else if (thalwil.In(lat_lon)) {
    printf("thalwil plan\n");
    fprintf(stderr, "thalwil plan");
    plan_.Build(thalwil_plan);
  } else if (horgen.In(lat_lon)) {
    printf("horgen plan\n");
    fprintf(stderr, "horgen plan");
    plan_.Build(horgen_plan);
  } else if (au.In(lat_lon)) {
    printf("au plan\n");
    fprintf(stderr, "au plan");
    plan_.Build(au_plan);
  } else if (waedenswil.In(lat_lon)) {
    printf("waedenswil plan\n");
    fprintf(stderr, "waedenswil plan");
    plan_.Build(waedenswil_plan);
  } else if (wollerau.In(lat_lon)) {
    printf("wollerau plan\n");
    fprintf(stderr, "wollerau plan");
    plan_.Build(wollerau_plan);
  } else {
    fprintf(stderr, "According to our GPS we are not on lake zuerich.");
    fprintf(stderr, "lat %8.6g lon %8.6g ", lat_lon.lat, lat_lon.lon);
    if (brest.In(lat_lon)) {
      fprintf(stderr, "According to our GPS we are near Brest.");
      fprintf(stderr, "lat %8.6g lon %8.6g ", lat_lon.lat, lat_lon.lon);
    } else {
      fprintf(stderr, "According to our GPS we are in the middle of our journey");
      fprintf(stderr, "lat %8.6g lon %8.6g", lat_lon.lat, lat_lon.lon);
    }
    fprintf(stderr, "Caribbean plan");
    plan_.Build(caribbean_plan);
    fprintf(stderr, "Built the Caribbean plan");
  }
}

double Planner::ToDeg(double lat_deg, double lon_deg) {
  //fprintf(stderr, "ToDeg %lg %lg\n", lat_deg, lon_deg);

  CHECK(!isnan(lat_deg));
  CHECK(!isnan(lon_deg));

  LatLon lat_lon(lat_deg, lon_deg);
  if (!initialized_) {
    Init(lat_lon);
    initialized_ = true;
    printf("initialized\n");
  }
  if (plan_.TargetReached(lat_lon)) {
    fprintf(stderr, "Hurray! Target Reached! CrissCrossing now.");
    fprintf(stderr, "On Target\n");
    if (NowSeconds() > last_turn_time_ + 120) {
      last_turn_time_ = NowSeconds();
      alpha_star_ = NormalizeDeg(alpha_star_ + 165);
      fprintf(stderr, "Turning by 165 to %f.", alpha_star_);
    } else {
      fprintf(stderr, "Not turning.");
    }
  } else {
    alpha_star_ = plan_.ToDeg(lat_deg, lon_deg);
  }
  return alpha_star_;
}

bool Planner::TargetReached(const LatLon& lat_lon){
  return plan_.TargetReached(lat_lon);
}
