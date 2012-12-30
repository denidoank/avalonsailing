// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

// Angles are represented as integer types.
// Wrap around happens naturally in the form of overflow.

#include "angle.h"

#include <math.h>
#include <stdio.h>

#include "check.h"


// utype is a 64 bit unsigned integer type such that addtion and subtraction do not overflow.
// The overflow of signed integer values is undefined behaviour, but for unsigned integers
// everything is standardized and portable.
const int Angle::MANTISSA = 64;  // i.e. bigger than the mantissa of a double variable.
const Angle::utype Angle::HALF_RANGE = 1ULL << (MANTISSA-1);  // 0x8000..00

const long double Angle::DEG_TO_ATYPE = Angle::HALF_RANGE / 180.0L;
const long double Angle::RAD_TO_ATYPE = Angle::HALF_RANGE / (long double)M_PI;
const long double Angle::ATYPE_TO_DEG = 180.0L / Angle::HALF_RANGE;
const long double Angle::ATYPE_TO_RAD = (long double)M_PI / Angle::HALF_RANGE;
const long double Angle::EPSILON_DEG = ATYPE_TO_DEG;
const long double Angle::EPSILON_RAD = ATYPE_TO_RAD;

namespace {
  // Work around a name clash.
  double math_sin(double x) {return sin(x);}
  double math_cos(double x) {return cos(x);}
  double math_atan2(double y, double x) {return atan2(y, x);}
}

Angle::Angle() : angle_(0) {}

Angle::Angle(int64_t angle) : angle_(static_cast<utype>(angle)) {
  // fprintf(stderr, "ctor  : 0x%16llx\n", this->angle_);
}

Angle Angle::fromDeg(int deg) {
  CHECK(-180 <= deg && deg < 360);
  atype x = DEG_TO_ATYPE * normalizeInputDeg(deg) + 0.5;
  return Angle(x);
}

Angle Angle::fromDeg(double deg) {
  CHECK(-180 <= deg && deg < 360);
  atype x = DEG_TO_ATYPE * normalizeInputDeg(deg) + 0.5;
  return Angle(x);
}

Angle Angle::fromDeg(long double deg) {
  CHECK(-180 <= deg && deg < 360);
  atype x = DEG_TO_ATYPE * normalizeInputDeg(deg) + 0.5;
  return Angle(x);
}

Angle Angle::fromRad(double rad) {
  // If someone tries to convert 10 rad then it is most probably a mixup with degrees.
  CHECK(-M_PI <= rad && rad < 2 * M_PI);
  return Angle(RAD_TO_ATYPE * normalizeInputRad(rad) + 0.5);
}

Angle Angle::fromRad(long double rad) {
  // If someone tries to convert 10 rad then it is most probably a mixup with degrees.
  CHECK(-M_PI <= rad && rad < 2 * M_PI);
  return Angle(RAD_TO_ATYPE * normalizeInputRad(rad) + 0.5);
}

void Angle::print() const {
  fprintf(stderr, "print:  %20.20Lf degrees (internally %16lld 0x%16llx)\n", ATYPE_TO_DEG * angle_, angle_, angle_);
}


double Angle::deg() const {
  return ATYPE_TO_DEG * static_cast<atype>(angle_);
}

double Angle::rad() const {
  return ATYPE_TO_RAD * static_cast<atype>(angle_);
}

double Angle::udeg() const {
  fprintf(stderr, "deg : %lld * %15Le = %15Le deg\n", angle_, ATYPE_TO_DEG, ATYPE_TO_DEG * angle_);
  return ATYPE_TO_DEG * angle_;
}

double Angle::urad() const {
  return ATYPE_TO_RAD * angle_;
}

// Over and underflow may occur here, but
// the angles stay normalized.
Angle& Angle::add(const Angle& left, const Angle& right) {
  this->angle_ = left.angle_ + right.angle_;
  fprintf(stderr, "add              : 0x%16llx\n", this->angle_);
  return *this;
}

Angle& Angle::sub(const Angle& left, const Angle& right) {
  this->angle_ = left.angle_ - right.angle_;
  fprintf(stderr, "sub              : 0x%16llx\n", this->angle_);
  return *this;
}

Angle& Angle::operator+= (const Angle& right) {
  //fprintf(stderr, "this  : 0x%16llx\nright : 0x%16llx\n", this->angle_, right.angle_);
  this->angle_ += right.angle_;
  //fprintf(stderr, "sum+= : 0x%16llx %16lld\n", this->angle_, this->angle_);
  return *this;
}

Angle Angle::operator+ (const Angle& right) const {
  Angle a(this->angle_ + right.angle_);
  return a;
}

Angle Angle::operator- (const Angle& right) const {
  Angle a(this->angle_ - right.angle_);
  return a;
}

Angle& Angle::operator-= (const Angle& right) {
  this->angle_ -= right.angle_;
  return *this;
}

// unary minus
Angle Angle::operator- () const {
  Angle a(0U - this->angle_);
  return a;
}

Angle Angle::opposite() const {
  Angle a(this->angle_ + HALF_RANGE);
  return a;
}

Angle Angle::operator/ (int divisor) const {
  CHECK(divisor != 0);
  Angle a(static_cast<atype>(this->angle_) / divisor);
  return a;
}

double Angle::sin() {
  return math_sin(rad());
}

double Angle::cos() {
  return math_cos(rad());
}

// as atan2 from math.h
Angle Angle::fromAtan2(double y, double x) {
  return Angle::fromRad(math_atan2(y, x));
}

bool Angle::operator==(const Angle& right) {
  return this->angle_ == right.angle_;
}


//overflow during multiplication defined or undefined behaviour?

// Force result into [-180, 180). template this !
int Angle::normalizeInputDeg(int alpha_deg) {
  if (alpha_deg >= 180)
    alpha_deg -= 360;
  return alpha_deg;
}

double Angle::normalizeInputDeg(double alpha_deg) {
  if (alpha_deg >= 180)
    alpha_deg -= 360;
  return alpha_deg;
}

long double Angle::normalizeInputDeg(long double alpha_deg) {
  if (alpha_deg >= 180)
    alpha_deg -= 360;
  return alpha_deg;
}

double Angle::normalizeInputRad(double alpha_rad) {
  if (alpha_rad >= M_PI)
    alpha_rad -= 2 * M_PI;
  return alpha_rad;
}

long double Angle::normalizeInputRad(long double alpha_rad) {
  if (alpha_rad >= M_PI)
    alpha_rad -= 2 * M_PI;
  return alpha_rad;
}



// Force result into [-180, 180).
int normalizeDeg(int alpha_deg) {
  while (alpha_deg >= 180)
    alpha_deg -= 360;
  while (alpha_deg < -180)
    alpha_deg += 360;
  return alpha_deg;
}

double normalizeDeg(double alpha_deg) {
  while (alpha_deg >= 180)
    alpha_deg -= 360;
  while (alpha_deg < -180)
    alpha_deg += 360;
  return alpha_deg;
}

// Force result into [-pi, pi).
double normalizeRad(double alpha_rad) {
  while (alpha_rad >= M_PI)
    alpha_rad -= 2 * M_PI;
  while (alpha_rad < -M_PI)
    alpha_rad += 2 * M_PI;
  return alpha_rad;
}

// Force result into [-180, 180).
int normalizeDegSaturated(int alpha_deg) {
  if (alpha_deg >= 180)
    alpha_deg = 179;
  else if (alpha_deg < -180)
    alpha_deg = -180;
  return alpha_deg;
}

double normalizeDegSaturated(double alpha_deg) {
  if (alpha_deg >= 180)
    alpha_deg = 180 - Angle::EPSILON_DEG;
  else if (alpha_deg < -180)
    alpha_deg = 180;
  return alpha_deg;
}

// Force result into [-pi, pi).
double normalizeRadSaturated(double alpha_rad) {
  if (alpha_rad >= M_PI)
    alpha_rad = M_PI - Angle::EPSILON_RAD;
  else if (alpha_rad < -M_PI)
    alpha_rad = -M_PI;
  return alpha_rad;
}
