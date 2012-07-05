// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011
#ifndef SKIPPER_LAT_LON_H
#define SKIPPER_LAT_LON_H

#include "common/check.h"

const static double to_cartesian_meters = 111111; // from degrees to meters distance, 1 degree is 111.1km at the equator.

// Latitude and longitude in degrees,
// latitude is the North component (South coordinates get a negative sign).
// longitude is the East component (West coordinates get a negative sign).
struct LatLon {
  LatLon(double latitude, double longitude) : lat(latitude), lon(longitude) {
    //fprintf(stderr, "LatLon %lg %lg\n", latitude, longitude);
    CHECK_IN_INTERVAL(-360, latitude, 360);
    CHECK_IN_INTERVAL(-360, longitude, 360);
  }
  void Flat(double* x, double* y) {
    *x = lat * to_cartesian_meters;
    *y = lon * to_cartesian_meters;
  }
  double lat;
  double lon;
};



#endif  // SKIPPER_LAT_LON_H
