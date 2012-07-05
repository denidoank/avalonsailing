// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011
#ifndef SKIPPER_LAT_LON_H
#define SKIPPER_LAT_LON_H

#include "common/check.h"
#include "common/convert.h"

const static double to_cartesian_meters = 111111.1; // from degrees to meters distance, 1 degree is 111.1km at the equator.

// Latitude and longitude in degrees,
// latitude is the North component (South coordinates get a negative sign).
// longitude is the East component (West coordinates get a negative sign).
struct LatLon {
  LatLon(double latitude, double longitude) : lat(latitude), lon(longitude) {
    // The approximations are too inaccurate near the poles.
    CHECK_IN_INTERVAL(-75, latitude, 75);
    CHECK_IN_INTERVAL(-180, longitude, 180);
  }
  void Flat(double* x, double* y) {
    *x = lat * to_cartesian_meters;
    // Not stretching the y dimension causes angular errors (10 deg for Zurich).
    *y = lon * to_cartesian_meters / cos(Deg2Rad(lat));
  }
  double lat;
  double lon;
};

#endif  // SKIPPER_LAT_LON_H
