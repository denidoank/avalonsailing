// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, August 2011

// This works for our planned route through the Atlantic,
// but nowhere else and probably not in 10 years from now.
// The error margin is 2 degrees.
// Interpolation would reduce the error margin.
#if 0
#include "correct_compass_angle.h"

#include <math.h>
#include <stdio.h>
#include <string>

#include "declination_data.h"

void  CorrectCompassAngle(double lat_deg, double lon_deg, double* compass_deg) {
  if (isnan(lat_deg) || isnan(lon_deg) ||
      lat_deg < lat_min || lat_deg > lat_max ||
      lon_deg < lon_min || lon_deg > lon_max) {
    fprintf(stderr, "You just fell off the edge at Lat %g deg, Lon: %g deg", lat_deg, lon_deg);
    // Or the GPS just failed.
    return;
  }

  // lat =  0  ... 50, 10+1 values, in 5 deg steps
  // lon = -60 ... 10, 14+1 values, in 5 deg steps
  const int    lat_rows = (lat_max - lat_min) / lat_step + 1.5;
  const int    lon_columns = (lon_max - lon_min) / lon_step + 1.5;

  i_lat = floor((lat - lat_min + lat_step/2) / (lat_max - lat_min));
  i_lon = floor((lon - lon_min + lon_step/2) / (lon_max - lon_min));
  CHECK_LE(0, i_lat);  // Sorry, you just fell off the map.
  CHECK_GE(lat_rows, i_lat);
  CHECK_LE(0, i_lon);
  CHECK_GE(lat_rows, i_lon);

  double declination = declination_data[i_lat * lon_columns + i_lon];
  *compass_deg -= declination;  // confirmed: Declination East is positive, Declination west is negative and angles go clockwise.
}

 
#endif






