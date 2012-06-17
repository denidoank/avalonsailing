// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011

#include <math.h>
#include <stdio.h>

#include "skipper/target_circle.h"
#include "common/convert.h"
#include "common/normalize.h"

// Cartesian input
TargetCircle::TargetCircle(double x0, double y0, double radius) :
    x0_(x0), y0_(y0), radius_squared_(radius * radius) {
  CHECK_GT(radius, 0);
}

// radius in degrees, equivalent to the lat-lon data, where 1 degree corresponds to 111km.
TargetCircle::TargetCircle(LatLon lat_lon, double radius_deg) {
  x0_ = lat_lon.lat;
  y0_ = lat_lon.lon;
  radius_squared_ =  radius_deg * radius_deg;
}

double TargetCircle::DistanceSquared(double x, double y) const {
  double delta_x = x0_ - x;
  double delta_y = y0_ - y;
  return delta_x * delta_x + delta_y * delta_y;
}

bool TargetCircle::In(double x, double y, double expansion) const {
  //printf("dist %lg deg\n", sqrt(DistanceSquared(x, y)));
  return DistanceSquared(x, y) <=
         radius_squared_ * expansion * expansion;
}

bool TargetCircle::In(LatLon lat_lon, double expansion) const {
  return DistanceSquared(lat_lon.lat, lat_lon.lon) <=
         radius_squared_ * expansion * expansion;
}

bool TargetCircle::In(const TargetCircle& t) const {
  return DistanceSquared(t.x0_, t.y0_) <= radius_squared_;
}

double TargetCircle::Distance(double x, double y) const {
  return sqrt(DistanceSquared(x, y));
}

double TargetCircle::ToDeg(double x, double y) const {
  double delta_x = x0_ - x;
  double delta_y = y0_ - y;
  if (delta_x == 0 && delta_y == 0)
    return 0;
  return NormalizeDeg(Rad2Deg(atan2(delta_y, delta_x)));
}

double TargetCircle::ToDeg(LatLon lat_lon) const {
  double delta_x = x0_ - lat_lon.lat;
  double delta_y = y0_ - lat_lon.lon;
  if (delta_x == 0 && delta_y == 0)
    return 0;
  return NormalizeDeg(Rad2Deg(atan2(delta_y, delta_x)));
}

double TargetCircle::x0() const {return x0_;}
double TargetCircle::y0() const {return y0_;}
double TargetCircle::radius() const {return sqrt(radius_squared_);}

