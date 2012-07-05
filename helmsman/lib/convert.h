// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
//
// Conversions between measurement units
#ifndef COMMON_CONVERT_H_
#define COMMON_CONVERT_H_

double Deg2Rad(double degree);
double Rad2Deg(double radians);

double NauticalMileToMeter(double miles);
double MeterToNauticalMile(double meters);

double KnotsToMeterPerSecond(double knots);
double MeterPerSecondToKnots(double mps);

#endif  // COMMON_CONVERT_H_
