// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "skipper/planner.h"
#include <stdio.h>

#include "helmsman/normal_controller.h"
#include "lib/fm/log.h"
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
  if (kilchberg.In(lat_lon)) {
    plan_.Build(kilchberg_plan);
  } else if (thalwil.In(lat_lon)) {
    printf("thalwil plan\n");
    FM_LOG_INFO("thalwil plan");
    plan_.Build(thalwil_plan);
  } else if (horgen.In(lat_lon)) {
    printf("horgen plan\n");
    FM_LOG_INFO("horgen plan");
    plan_.Build(horgen_plan);
  } else if (au.In(lat_lon)) {
    printf("au plan\n");
    FM_LOG_INFO("au plan");
    plan_.Build(au_plan);
  } else if (waedenswil.In(lat_lon)) {
    printf("waedenswil plan\n");
    FM_LOG_INFO("waedenswil plan");
    plan_.Build(waedenswil_plan);
  } else if (wollerau.In(lat_lon)) {
    printf("wollerau plan\n");
    FM_LOG_INFO("wollerau plan");
    plan_.Build(wollerau_plan);
  } else {
    FM_LOG_INFO("According to our GPS we are not on lake zuerich.");
    FM_LOG_INFO("lat %8.6g lon %8.6g ", lat_lon.lat, lat_lon.lon);
    if (brest.In(lat_lon)) {
      FM_LOG_INFO("According to our GPS we are near Brest.");
      FM_LOG_INFO("lat %8.6g lon %8.6g ", lat_lon.lat, lat_lon.lon);
    } else {
      FM_LOG_INFO("According to our GPS we are in the middle of our journey");
      FM_LOG_INFO("lat %8.6g lon %8.6g", lat_lon.lat, lat_lon.lon);
    }
    FM_LOG_INFO("Caribbean plan");
    plan_.Build(caribbean_plan);
    FM_LOG_INFO("Built the Caribbean plan");
  }
}

double Planner::ToDeg(double lat_deg, double lon_deg) {
  LatLon lat_lon(lat_deg, lon_deg);
  if (!initialized_) {
    Init(lat_lon);
    initialized_ = true;
  }
  if (plan_.TargetReached(lat_lon)) {
    FM_LOG_WARN("Hurray! Target Reached! CrissCrossing now.");
    if (NowSeconds() > last_turn_time_ + 120) {
      last_turn_time_ = NowSeconds();
      alpha_star_ = NormalizeDeg(alpha_star_ + 165);
      FM_LOG_INFO("Turning by 165 to %f.", alpha_star_);
    } else {
      FM_LOG_INFO("Not turning.");
    }
  }
  alpha_star_ = plan_.ToDeg(lat_deg, lon_deg);
  return alpha_star_;
}

bool Planner::TargetReached(const LatLon& lat_lon){
  return plan_.TargetReached(lat_lon);
}
