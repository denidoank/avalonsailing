// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Ivan Zauharodneu, Aug 2011

#include "util.h"

#include <iostream>

namespace skipper {
namespace {
// Since Earth is not a perfect sphere, there is no single correct radius. This
// value is a mean radius.
static const double kEarthRadius = 6371009.0;  // meters
}

void SphericalShortestPath(const LatLon& from, const LatLon& to,
                           Bearing* bearing, double* distance_m) {
  double dlat = to.lat_rad() - from.lat_rad();
  double dlon = to.lon_rad() - from.lon_rad();
  double a = sin(dlat/2)*sin(dlat/2) +
             cos(from.lat_rad())*cos(to.lat_rad())*sin(dlon/2)*sin(dlon/2);

  *distance_m = kEarthRadius * 2 * atan2(sqrt(a), sqrt(1-a));
  double rad = atan2(
      sin(dlon)*cos(to.lat_rad()),
      cos(from.lat_rad())*sin(to.lat_rad()) -
      sin(from.lat_rad())*cos(to.lat_rad())*cos(dlon));
  if (fabs(rad) > 10) fprintf(stderr, "SphericalShortestPath: angle_in_rad: %lf\n", rad);
  *bearing = Bearing::Radians(rad);
}

LatLon SphericalMove(const LatLon& from, Bearing bearing, double distance_m) {
  CHECK(fabs(from.lat_deg()) < 80);  // Singularities near the poles.
  fprintf(stderr, "SphericalMove");
  double dist = distance_m / kEarthRadius;
  double sin_lat = sin(from.lat_rad());
  double cos_lat = cos(from.lat_rad());
  fprintf(stderr, "SphericalMove %lf %lf %lf\n", dist, sin_lat, cos_lat);
  // lat2 can be nan for "from" near the poles.
  double lat2 = asin(sin_lat*cos(dist) + cos_lat*sin(dist)*cos(bearing.rad()));
  CHECK(!isnan(lat2));
  double dlon = atan2(sin(bearing.rad())*sin(dist)*cos_lat,
                      cos(dist)-sin_lat*sin(lat2));
  double lon2 = from.lon_rad() + dlon;
  fprintf(stderr, "SphericalMove dlon, lat2, lon2: %lf %lf %lf\n", dlon, lat2, lon2);
  return LatLon(lat2, lon2);
}

double MinDistance(Bearing a, double u,
                   Bearing b, double v,
                   Bearing a_b, double distance_a_b,
                   double time_window_s) {
  //CHECK_GE(u, 0);
  //CHECK_GE(v, 0);
  //CHECK_GE(distance_a_b, 0);
  //CHECK_GE(time_window_s, 0);

  // Both stationary
  if (u < 1e-9 && v < 1e-9) {
    return distance_a_b;
  }

  double sin_alpha = sin(a_b.rad() - a.rad());
  double cos_alpha = cos(a_b.rad() - a.rad());
  double sin_beta = sin(b.rad() - a_b.rad() - M_PI);
  double cos_beta = cos(b.rad() - a_b.rad() - M_PI);

  double px = v*cos_beta + u*cos_alpha;
  double py = v*sin_beta - u*sin_alpha;
  double t;  // moment of minimum distance

  if (fabs(sin_alpha * cos_beta + sin_beta * cos_alpha) < 1e-9) {
    // A and B are moving in parallel directions
    double v_proj = u + v*(cos_alpha*cos_beta - sin_alpha*sin_beta);
    if (fabs(v_proj) < 1e-9) {
      // Same speed and direction
      return distance_a_b;
    } else {
      t = cos_alpha * distance_a_b / v_proj;
    }
  } else {
    t = distance_a_b * px / (px*px + py*py);
  }

  t = std::min(time_window_s, std::max(0.0, t));

  double dx = distance_a_b - t*px;
  double dy = t*py;
  return sqrt(dx*dx + dy*dy);
}
}  // skipper
