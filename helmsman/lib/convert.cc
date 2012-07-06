// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Conversions between measurement units

#include <math.h>
#include "convert.h"

// Angle
double Deg2Rad(double degree) {
  return(degree * M_PI / 180.0);
}

double Rad2Deg(double rad) {
  return(rad / M_PI * 180);
}

// Distance
const double kOneNauticalMileInMeters = 1852.0;

double MeterToNauticalMile(double meters) {
  return meters / kOneNauticalMileInMeters;
}

double NauticalMileToMeter(double miles) {
  return miles * kOneNauticalMileInMeters;
}

// Speed
double KnotsToMeterPerSecond(double knots) {
  return NauticalMileToMeter(knots) / 3600.0;
}

double MeterPerSecondToKnots(double mps) {
  return MeterToNauticalMile(mps) * 3600.0;
}




