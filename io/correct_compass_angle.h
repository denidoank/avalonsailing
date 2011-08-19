// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// Sail brake status is managed by the drive interface.
// All angle values in degrees. For the drives the [-180, 180] convention
// is used.
// Constructors, Reset, Check and conversion functions are provided for.

#ifndef IO_CORRECT_COMPASS_ANGLE_H
#define IO_CORRECT_COMPASS_ANGLE_H

// Given the current position, the compass_deg bearing is corrected to the
// value in relation to true North.
// If the coordinates are Nan or outside of the expected range, we do nothing.
#ifdef __CPP__
extern "C" {
#endif
void CorrectCompassAngle(double lat_deg, double lon_deg, double* compass_deg);
#ifdef __CPP__
};
#endif

#endif  // IO_CORRECT_COMPASS_ANGLE_H
