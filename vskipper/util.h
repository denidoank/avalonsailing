// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Ivan Zauharodneu, Aug 2011

#ifndef VSKIPPER_UTIL_H
#define VSKIPPER_UTIL_H

#include <math.h>
#include <stdio.h>
#include "common/normalize.h"
#include "common/check.h"

namespace skipper {

// 0 is North, increases clockwise.
class Bearing {
 public:
  Bearing() : rad_(0) {}
  static Bearing Degrees(double deg) { return Bearing(NormalizeRad(deg / 180.0 * M_PI)); }
  static Bearing Radians(double rad) {
    CHECK(-10 < rad && rad < 10);
    return Bearing(NormalizeRad(rad)); }
  static Bearing West() { return Bearing::Degrees(270); }

  double deg() const { return rad_ * 180.0 / M_PI; }
  double rad() const { return rad_; }

 private:
  explicit Bearing(double rad) : rad_(rad) {}
  double rad_;
};

class LatLon {
 public:
  LatLon() : lat_(0), lon_(0) {}
  LatLon(double lat, double lon) : lat_(lat), lon_(lon) {
    CHECK(-10 < lat && lat < 10);
    CHECK(-10 < lon && lon < 10);
  }
  static LatLon Degrees(double lat, double lon) {
    return LatLon(lat / 180.0 * M_PI, lon / 180.0 * M_PI); 
  }

  double lat_deg() const { return SymmetricDeg(lat_ * 180.0 / M_PI); }
  double lon_deg() const { return SymmetricDeg(lon_ * 180.0 / M_PI); }
  double lat_rad() const { return lat_; }
  double lon_rad() const { return lon_; }
  void print() {
    fprintf(stderr, "LatLon: [%.7lf %.7lf]", lat_deg(), lon_deg());
  }

 private:
  // radians
  double lat_;
  double lon_;
};

// Computes shortest path between two points on Earth. Returns bearing in
// @from -> @to direction plus distance.
//
// Note, precision is ony decent for relatively short distances (a few 100 km).
// Also as the name suggests, it assumes Earth is a sphere, not geoid.
void SphericalShortestPath(const LatLon& from, const LatLon& to,
                           Bearing* bearing, double* distance_m);

// Inverse of the function above: moves from starting position in @from in
// direction @bearing for @distance_m meters and returns resulting position.
//
// Note, similary to previous function, only works well for short distances.
LatLon SphericalMove(const LatLon& from, Bearing bearing, double distance_m);

// Given bearings and velocities of two ships as well as their relative position
// (bearing of b relative to a and distance) returns minimum distance these
// ships will have between them during the next time_window_s seconds
// (everything is in meters, seconds or meters/second).
//
// Warning, this function assumes velocities, distance and time window to have
// non-negative values. Just like the functions above, only works well for short
// distances.
double MinDistance(Bearing a, double velocity_a,
                   Bearing b, double velocity_b,
                   Bearing a_b, double distance_a_b,
                   double time_window_s);

}  // skipper

#endif  // VSKIPPER_UTIL_H
