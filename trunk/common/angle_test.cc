// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
//
// Unit test for common/angle
#include "common/angle.h"

#include <math.h>
#include "lib/testing/testing.h"

// Free functions that cope with everything.
ATEST(AngleTest, normalizeDeg) {
  EXPECT_EQ(0, normalizeDeg(-10800000.0));
  EXPECT_EQ(0, normalizeDeg(-108000.0));
  EXPECT_EQ(0, normalizeDeg(-1080.0));
  EXPECT_EQ(0, normalizeDeg(-720.0));
  EXPECT_EQ(0, normalizeDeg(-360.0));
  EXPECT_EQ(0, normalizeDeg(0.0));
  EXPECT_EQ(0, normalizeDeg(360.0));
  EXPECT_EQ(0, normalizeDeg(720.0));
  EXPECT_EQ(0, normalizeDeg(1080.0));
  EXPECT_EQ(0, normalizeDeg(108000.0));
  EXPECT_EQ(0, normalizeDeg(10800000.0));

  EXPECT_FLOAT_EQ(-1, normalizeDeg(-10800001));
  EXPECT_FLOAT_EQ( 1, normalizeDeg( 10800001));
}

ATEST(AngleTest, normalizeRad) {
  const double eps = 0.01;
  // The cases commented out work fine but fail with the |rad| < 10 check.
  EXPECT_FLOAT_EQ(-eps, normalizeRad(-300 * 2 * M_PI - eps));
  EXPECT_FLOAT_EQ(0,    normalizeRad(-300 * 2 * M_PI));
  EXPECT_FLOAT_EQ(0,    normalizeRad( -30 * 2 * M_PI));
  EXPECT_FLOAT_EQ(-eps, normalizeRad(  -3 * 2 * M_PI - eps));
  EXPECT_FLOAT_EQ(0,    normalizeRad(  -3 * 2 * M_PI));
  EXPECT_FLOAT_EQ(0,    normalizeRad(  -2 * 2 * M_PI));
  EXPECT_FLOAT_EQ(0,    normalizeRad(  -1 * 2 * M_PI));
  EXPECT_FLOAT_EQ(0,    normalizeRad(   0 * 2 * M_PI));
  EXPECT_FLOAT_EQ(-eps, normalizeRad(   0 * 2 * M_PI - eps));
  EXPECT_FLOAT_EQ(0,    normalizeRad(   1 * 2 * M_PI));
  EXPECT_FLOAT_EQ(-eps, normalizeRad(   1 * 2 * M_PI - eps));
  EXPECT_FLOAT_EQ(0,    normalizeRad(   2 * 2 * M_PI));
  EXPECT_FLOAT_EQ(-eps, normalizeRad(   2 * 2 * M_PI - eps));
  EXPECT_FLOAT_EQ(0,    normalizeRad(   3 * 2 * M_PI));
  EXPECT_FLOAT_EQ(0,    normalizeRad(  30 * 2 * M_PI + 1E-6));
  EXPECT_FLOAT_EQ(0,    normalizeRad( 300 * 2 * M_PI + 1E-6));
  EXPECT_FLOAT_EQ( eps, normalizeRad( 300 * 2 * M_PI + eps));
}

// All later tests assume angles in [-180, 360) as input.
ATEST(AngleTest, fromDeg) {
  EXPECT_EQ( 0, Angle::fromDeg(0.0).deg());
  EXPECT_FLOAT_EQ( 180/1000.0, Angle::fromDeg(180.0/1000.0).deg());
  EXPECT_FLOAT_EQ( 180/100.0,  Angle::fromDeg(180.0/100.0).deg());
  EXPECT_FLOAT_EQ( 180/10.0,   Angle::fromDeg(180.0/10.0).deg());
  EXPECT_FLOAT_EQ( 179.999999, Angle::fromDeg(179.999999).deg());
  EXPECT_FLOAT_EQ(-180/1000.0, Angle::fromDeg(-180.0/1000.0).deg());
  EXPECT_FLOAT_EQ(-180/100.0,  Angle::fromDeg(-180.0/100.0).deg());
  EXPECT_FLOAT_EQ(-180/10.0,   Angle::fromDeg(-180.0/10.0).deg());
  EXPECT_FLOAT_EQ(-179.999999, Angle::fromDeg(-179.999999).deg());

  EXPECT_FLOAT_EQ( 45.0, Angle::fromDeg( 45).deg());
  EXPECT_FLOAT_EQ( 45.0, Angle::fromDeg( 45.0).deg());
  EXPECT_FLOAT_EQ( 45.0, Angle::fromDeg( 45.0L).deg());
  EXPECT_FLOAT_EQ(-45.0, Angle::fromDeg(-45).deg());
  EXPECT_FLOAT_EQ(-45.0, Angle::fromDeg(-45.0).deg());
  EXPECT_FLOAT_EQ(-45.0, Angle::fromDeg(-45.0L).deg());

  EXPECT_EQ(-180,    Angle::fromDeg(-180).deg());
  EXPECT_EQ( 179.99, Angle::fromDeg(179.99).deg());
  EXPECT_EQ(-180,    Angle::fromDeg(180.0).deg());
  EXPECT_EQ(-179.99, Angle::fromDeg(180.01).deg());

  EXPECT_EQ(-179.99, Angle::fromDeg(-179.99).deg());
  EXPECT_EQ(-180,    Angle::fromDeg(-180.0).deg());

  EXPECT_EQ(          0, Angle::fromDeg(0.0).deg());
  EXPECT_FLOAT_EQ(-0.01, Angle::fromDeg(359.99).deg());
  EXPECT_FLOAT_EQ( 0.01, Angle::fromDeg(0.01).deg());

  EXPECT_EQ_TOL(-160,    Angle::fromDeg(200).deg(), 1E-9);
  EXPECT_EQ_TOL(  40,    Angle::fromDeg(normalizeDeg(400)).deg(), 1E-9);

  EXPECT_EQ_TOL( 160,    Angle::fromDeg(normalizeDeg(-200)).deg(), 1E-9);
  EXPECT_EQ_TOL( -40,    Angle::fromDeg(normalizeDeg(-400)).deg(), 1E-9);
}

ATEST(AngleTest, fromRad) {
  // [-M_PI, M_PI)
  const double eps = 0.01;
  EXPECT_FLOAT_EQ( M_PI - eps, Angle::fromRad(M_PI - eps).rad());
  EXPECT_FLOAT_EQ(-M_PI,       Angle::fromRad(M_PI).rad());
  EXPECT_FLOAT_EQ(-M_PI + eps, Angle::fromRad(M_PI + eps).rad());

  EXPECT_FLOAT_EQ(-M_PI + eps, Angle::fromRad(-M_PI + eps).rad());
  EXPECT_FLOAT_EQ(-M_PI,       Angle::fromRad(-M_PI).rad());

  EXPECT_FLOAT_EQ(0,           Angle::fromRad(0.0).rad());
  EXPECT_FLOAT_EQ(-eps,        Angle::fromRad(2 * M_PI - eps).rad());
}

ATEST(AngleTest, Add) {
  Angle a = Angle::fromDeg(-180.00);
  Angle b = Angle::fromDeg(179.999999999999);
  b += a;
  b.print();
  EXPECT_FLOAT_EQ(          -0.000000000001, b.deg());
  Angle c = b;
  EXPECT_FLOAT_EQ(          -0.000000000001, c.deg());
  c = a + Angle::fromDeg(  179.999999999999);
  c.print();
  EXPECT_FLOAT_EQ(          -0.000000000001, c.deg());
}

ATEST(AngleTest, Add2) {
  fprintf(stderr, "Add test\n\n");

  const double eps = 1.0E-12;
  const double tol = 5.0E-18;  // 60 bits should be good for 1E-18 precision.
  // +=
  Angle x = Angle::fromDeg(0);
  x += Angle::fromDeg(0);
  EXPECT_EQ_TOL(0, x.deg(), tol);
  x = Angle::fromDeg(0);
  x += Angle::fromDeg(170);
  EXPECT_EQ_TOL(170, x.deg(), tol);
  x = Angle::fromDeg(0);
  x += Angle::fromDeg(-170);
  EXPECT_EQ_TOL(-170, x.deg(), tol);
  x = Angle::fromDeg(0);
  x += Angle::fromDeg(-180.0/256);
  EXPECT_EQ_TOL(-180.0/256, x.deg(), tol);

  // Test error accumulation. 1 iterations changes y by -1 degrees.
  Angle y;
  int fraction = 256;
  Angle almost180 = Angle::fromDeg(180.0L - 180.0L / fraction);
  for (int n = 0; n < 10 * 2 * fraction; ++n) {
    y += almost180;
    if (n % (2 * fraction) == (2 * fraction - 1)) {
      y.print();
    }
  }
  EXPECT_EQ_TOL(0, y.deg(), 1 * 2 * fraction * tol);

  Angle a = Angle::fromDeg(-180.00);
  Angle b = Angle::fromDeg(180 - eps);
  b += a;
  b.print();
  EXPECT_FLOAT_EQ(-eps, b.deg());
  Angle c = b;
  EXPECT_FLOAT_EQ(-eps, c.deg());
  c = a + Angle::fromDeg(180 - eps);
  EXPECT_FLOAT_EQ(-eps, c.deg());
}

ATEST(AngleTest, UnaryMinus) {
  Angle b = Angle::fromDeg(179.99);
  Angle a = -b;
  EXPECT_EQ(-179.99, a.deg());
  b += -a;
  EXPECT_FLOAT_EQ(-0.02, b.deg());  // 179.99+179.99
  Angle c;
  c = -c;
  EXPECT_EQ(0, c.deg());
}

ATEST(AngleTest, Opposite) {
  Angle c;
  c = c.opposite();
  EXPECT_EQ(-180, c.deg());
  Angle d = Angle::fromDeg(179.99);
  EXPECT_FLOAT_EQ(-0.01, d.opposite().deg());
}

ATEST(AngleTest, Div) {
  Angle c;
  c = c / 2;
  EXPECT_EQ(0, c.deg());
  Angle d = Angle::fromDeg(179.99);
  c = d / 2;
  EXPECT_FLOAT_EQ(89.995, c.deg());
  d = Angle::fromDeg(-179.99);
  c = d / 2;
  EXPECT_FLOAT_EQ(-89.995, c.deg());
}

ATEST(AngleTest, Trigo) {
  Angle c;
  for (int n = 0; n < 360; ++n) {
    c += Angle::fromDeg(1);
    double cx = c.cos();
    double cy = c.sin();
    EXPECT_FLOAT_EQ(1, cx * cx + cy * cy);
    Angle reproduced = Angle::fromAtan2(cy, cx);
    EXPECT_FLOAT_EQ(0, (c - reproduced).deg());
  }
  c = Angle::fromAtan2(0, -1);
  EXPECT_FLOAT_EQ(-180, c.deg());
  c = Angle::fromAtan2(1, -1);
  EXPECT_FLOAT_EQ(135, c.deg());
  c = Angle::fromAtan2(1, 0);
  EXPECT_FLOAT_EQ(90, c.deg());
  c = Angle::fromAtan2(1, 1);
  EXPECT_FLOAT_EQ(45, c.deg());
  c = Angle::fromAtan2(0, 0);
  EXPECT_FLOAT_EQ(0, c.deg());
  c = Angle::fromAtan2(-1, -1);
  EXPECT_FLOAT_EQ(-135, c.deg());
}


int main(int argc, char* argv[]) {
  return testing::RunAllTests();
}
