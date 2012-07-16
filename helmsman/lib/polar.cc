// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "polar.h"

#include <math.h>
#include <stdio.h>

#include "check.h"
#include "convert.h"
#include "normalize.h"

Polar::Polar(double alpha_rad, double mag)
    : alpha_(alpha_rad),
      mag_(mag),
      x_(0),
      y_(0),
      cartesian_(false) {
  CHECK_IN_INTERVAL(-10, alpha_rad, 10);  // probably degree input
  if (mag_ == 0)
    alpha_ = 0;
  alpha_ = SymmetricRad(alpha_);
}

Polar Polar::operator+(const Polar& b) const {
  MakeCartesian();
  double x = x_ + b.mag_ * cos(b.alpha_);
  double y = y_ + b.mag_ * sin(b.alpha_);
  double mag = sqrt(x * x + y * y);
  double alpha;
  if (mag_ > 0)
    alpha = atan2(y, x);
  else
    alpha = 0;
  return Polar(alpha, mag);
}

Polar Polar::operator-(const Polar& b) const {
  MakeCartesian();
  double x = x_ - b.mag_ * cos(b.alpha_);
  double y = y_ - b.mag_ * sin(b.alpha_);
  double mag = sqrt(x * x + y * y);
  double alpha;
  if (mag_ != 0)
    alpha = atan2(y, x);
  else
    alpha = 0;
  return Polar(alpha, mag);
}

bool Polar::operator!=(const Polar& b) const {
  return alpha_ != b.alpha_ || mag_ != b.mag_;
}

const Polar& Polar::operator=(const Polar& r) {
    alpha_ = r.alpha_;
    mag_ = r.mag_;
    x_ = r.x_;
    y_ = r.y_;
    cartesian_ = r.cartesian_;
    return *this;
}

void Polar::Print(const char* name) const {
  printf("%s: %6.2g at %6.2f deg\n", name, mag_, Rad2Deg(alpha_));
}

double Polar::AngleRad() const {
  return alpha_;
}
double Polar::Mag() const {
  return mag_;
}

void Polar::MakeCartesian() const {
  if (!cartesian_) {
    x_ = mag_ * cos(alpha_);
    y_ = mag_ * sin(alpha_);
    cartesian_ = true;
  }
}

