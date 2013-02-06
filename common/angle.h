// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

// The Angle class is an efficient and easy to use representation of angles.
// If angles are stored as floating point types then he following problems
// occur:
// a) It is not clear in which unit the angles are given.
//    Interface errors are not unlikely.
// b) Depending on the application an unsigned presentation [0, 360) degrees
//    (for bearings in navigation, where the minus sign would be an additional
//    source of errors or as argument to trigonometric functions) or a signed one
//    [-180, 180) degrees (for bearing deviations or rudder angles) is preferred.
// c) Every mathematical operation on angles can lead to an overflow or underflow.
//    350 degrees plus 15 degrees is not 365 degrees, but 5 degrees. So after each
//    operation the overflow and underflow have to be handled.
// These problems are adressed as follows:
// a) To create a non-zero Angle object free methods (rad(), deg()) are provided.
//    For output as double values methods with appropriate names are provided.
// b) This problem is relevant for output only. The input methods accept [-180, 360) degrees.
//    2 output methods (signed and unsigned) are provided.
// c) The internal representation of the angle supports the wrap around efficiently.
//    The user has to provide angles in the legal range. If angles e.g. >= 360 degrees
//    are possible then the user has to normalize them (i.e. map into the legal range)
//    first. Otherwise the Angle creation will fail with an assertion error.
// The precision of operations on Angle objects is typically as good as with double
// values.

// Angle shalll be used as a POD, i.e. all function parameters shall be Angle, not
// const Angle& .
// Precision: An Angle is more precise than a double. For less storage and
// faster processing replace the internal types by their 32 bit equivalents!
// Performance:
// Using double requires a floating point operation and a remainder calculation.
// With Angles the operations are done as integer operations.

/* These I want to get rid of:
// Force angle into [0, 360).
int NormalizeDeg(int alpha_deg);
double NormalizeDeg(double alpha_deg);
long double NormalizeDeg(long double alpha_deg);

// Force result into [-180, 180).
int SymmetricDeg(int alpha_deg);
double SymmetricDeg(double alpha_deg);
long double SymmetricDeg(long double alpha_deg);

// Force radians into [0, 2*pi).
double NormalizeRad(double alpha_rad);
// Force result into [-pi, pi).
double SymmetricRad(double alpha_rad);
*/
#ifndef COMMON_ANGLE_H_
#define COMMON_ANGLE_H_

#include <iostream>
#include <stddef.h>
#include <stdint.h>

#include "../common/check.h"

// If the range of x is actually undefined use
// Angle::fromDeg(normalizeDeg(x));
// Map the input value into [-180, 180) degrees.
// -190 degrees are mapped to 170 degrees.
// I.e. for the return value x
// -180 degrees <= x < 180 degrees
// is aways true.
int normalizeDeg(int alpha_deg);
double normalizeDeg(double alpha_deg);
long double normalizeDeg(long double alpha_deg);

// Map the input value into [-pi, pi) radians.
double normalizeRad(double alpha_rad);

// If the extreme values shall be mapped
// onto the nearest limits of the range, then
// use these functions.
// -190 degrees are clipped to -180 degrees.
int normalizeDegSaturated(int alpha_deg);
double normalizeDegSaturated(double alpha_deg);

// Map the input value into [-pi, pi) radians.
double normalizeRadSaturated(double alpha_rad);

class Angle {

public:
  friend Angle deg(int deg);
  friend Angle deg(double deg);
  friend Angle deg(long double deg);

  friend Angle rad(int rad);
  friend Angle rad(double rad);
  friend Angle rad(long double rad);

  // as atan2 from math.h
  // N.B. atan2 has a value range (-pi, pi] but
  // we work with [-pi, pi).
  friend Angle fromAtan2(double y, double x);


  Angle();  // Intentionally there is no constructor with an input value.
            // Use deg() or rad()!

  // Requires that zero is 0.
  Angle& operator= (int zero);

  // Overloaded operators all work as if the angles were signed double angles,
  // with the addition that the wrap around at +180 degrees / 180 degrees is
  // handled.
  Angle& add(const Angle& left, const Angle& right);
  Angle& sub(const Angle& left, const Angle& right);
  Angle operator+ (const Angle& right) const;
  Angle& operator+=(const Angle& right);
  Angle operator- (const Angle& right) const;
  Angle& operator-=(const Angle& right);

  // unary minus, as ususal implies signed representation.
  Angle operator- () const;
  // implies signed representation, i.e. -160 / 2 = -80 .
  // Check-fails for divisor == 0.
  Angle operator/ (int divisor) const;
  // implies signed representation, i.e. -160 * 0.5 = -80 .
  // Check-fails for factor > 1, i.e. whenever overflow might occur.
  Angle operator* (double factor) const;
  Angle operator* (int factor) const;  // checkfails for |factor| > 1

  double sin() const;
  double cos() const;

  // The following methods imply a signed angle interpretation.
  int sign() const;
  int signNotZero() const;
  bool positive() const;
  bool negative() const;
  bool zero() const;

  // So (-179degrees > 179 degrees) is true.
  // By definition (-180degrees < 0 degrees) is false,
  // and -90deg < 90deg is false also.
  // "less than" is to be interpreted as "left of".
  bool operator< (const Angle& right) const;
  bool operator==(const Angle& right) const;
  inline bool operator!=(const Angle& right) const {return !operator==(right);}
  inline bool operator> (const Angle& right) const {return right.operator< (*this);}
  inline bool operator<=(const Angle& right) const {return !operator> (right);}
  inline bool operator>=(const Angle& right) const {return !operator< (right);}

  inline bool operator< (int zero) const {
    CHECK_EQ(0, zero);
    return negative();
  }

  inline bool operator> (int zero) const {
    CHECK_EQ(0, zero);
    return positive();
  }

  inline bool operator==(int zero) const {
    CHECK_EQ(0, zero);
    return angle_ == 0;
  }

  // Returns the opposite direction, i.e. this + 180 degrees.
  Angle opposite() const;

  // The absolute magnitude of the angle when interpreed as
  // a signed value.
  Angle abs() const;

  // The nearest of two options from this. Prefer option1 if the distances are equal.
  Angle nearest(Angle option1, Angle option2, bool* took_option1 = NULL) const;


  void print(const char* label = "") const;

  // Returns the angle in the desired unit.
  // signed degrees, in [-180, 180), i.e. -180 <= x.deg() < 180
  double deg() const;
  // signed radians, in [-pi, pi)
  double rad() const;
  // unsigned degrees, in [0, 360)
  double udeg() const;
  // unsigned radians, in [0, 2*pi)
  double urad() const;

  const static long double EPSILON_DEG;
  const static long double EPSILON_RAD;

private:
  typedef int64_t atype;
  typedef uint64_t utype;

  explicit Angle(atype angle);

  static int normalizeInputDeg(int alpha_deg);
  static double normalizeInputDeg(double alpha_deg);
  static long double normalizeInputDeg(long double alpha_deg);

  static double normalizeInputRad(double alpha_rad);
  static long double normalizeInputRad(long double alpha_rad);

  const static int MANTISSA;             // length in bits
  const static utype HALF_RANGE;
  // Why long double? The output conversion would be too imprecise with double.
  const static long double DEG_TO_ATYPE;
  const static long double RAD_TO_ATYPE;
  const static long double ATYPE_TO_DEG;
  const static long double ATYPE_TO_RAD;

  utype angle_;
};

// For any input guaranteed to be within [-180, 360) use this:
Angle deg(int deg);
Angle deg(double deg);
Angle deg(long double deg);

Angle rad(int radians);
Angle rad(double radians);
Angle rad(long double radians);

// as atan2 from math.h
// N.B. atan2 has a value range (-pi, pi] but
// we work with [-pi, pi).
Angle fromAtan2(double y, double x);

// Force any angle into [0, 360).
int NormalizeDeg(int alpha_deg);
double NormalizeDeg(double alpha_deg);
long double NormalizeDeg(long double alpha_deg);

// Force any angle into [-180, 180)
int SymmetricDeg(int alpha_deg);
double SymmetricDeg(double alpha_deg);
long double SymmetricDeg(long double alpha_deg);

// Force any radians into [0, 2*pi).
double NormalizeRad(double alpha_rad);

// Force any radians angle into [-pi, pi)
double SymmetricRad(double alpha_rad);

// To make our test check macros work.
std::ostream& operator<<(std::ostream& os, const Angle& obj);

inline bool operator< (int zero, const Angle& right) {
  CHECK_EQ(0, zero);
  return right.negative();
}

inline bool operator== (int zero, const Angle& right) {
  CHECK_EQ(0, zero);
  return right.zero();
}

inline bool operator!= (int zero, const Angle& right) {
  CHECK_EQ(0, zero);
  return !right.zero();
}

inline bool operator> (int zero, const Angle& right) {
  CHECK_EQ(0, zero);
  return right.positive();
}

#define EXPECT_ANGLE_EQ(a, b) \
  do { \
    if (fabs(((a) - (b)).deg()) > 1E-10) { \
      std::cout << __FILE__ << ":" <<__LINE__ \
      << "\nTest a == b (tol:1E-10 degree) failed with expected:\n"; \
      std::cout << #a " which is " << std::setprecision(16) << (a.deg()) << "\n"; \
      std::cout << "versus actual:\n"; \
      std::cout << #b " which is " << (b.deg()) << "\n"; \
      exit(1); \
    } \
  } while(0)

#endif  // COMMON_ANGLE_H_
