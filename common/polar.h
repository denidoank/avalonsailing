// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// A small class to handle a vector in polar representation. Incomplete.
// Trigonometric argument (or angle) in radians.

#ifndef COMMON_POLAR_H
#define COMMON_POLAR_H

#include <math.h>
#include <stdio.h>

#include "common/angle.h"
#include "common/check.h"
#include "common/convert.h"

class Polar {
 public:
  Polar(double alpha_rad, double mag);
  Polar(Angle alpha, double mag);
  Polar operator+(const Polar& b) const;
  Polar operator-(const Polar& b) const;
  bool operator!=(const Polar& b) const;
  const Polar& operator=(const Polar& r);
  void Print(const char* name) const;
  double AngleRad() const;
  // argument ( = angle )
  Angle Arg() const;
  // magnitude
  double Mag() const;
  
 private:
  void MakeCartesian() const;
  
  double alpha_;
  double mag_;
  mutable double x_;
  mutable double y_;
  mutable bool cartesian_;
};

#endif  // COMMON_POLAR_H
