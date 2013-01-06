// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
//
// Unit test for common/angle
#include "common/angle.h"

#include <math.h>
#include "common/convert.h"
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
  EXPECT_EQ( 0, deg(0.0).deg());
  EXPECT_FLOAT_EQ( 180/1000.0, deg(180.0/1000.0).deg());
  EXPECT_FLOAT_EQ( 180/100.0,  deg(180.0/100.0).deg());
  EXPECT_FLOAT_EQ( 180/10.0,   deg(180.0/10.0).deg());
  EXPECT_FLOAT_EQ( 179.999999, deg(179.999999).deg());
  EXPECT_FLOAT_EQ(-180/1000.0, deg(-180.0/1000.0).deg());
  EXPECT_FLOAT_EQ(-180/100.0,  deg(-180.0/100.0).deg());
  EXPECT_FLOAT_EQ(-180/10.0,   deg(-180.0/10.0).deg());
  EXPECT_FLOAT_EQ(-179.999999, deg(-179.999999).deg());

  EXPECT_FLOAT_EQ( 45.0, deg( 45).deg());
  EXPECT_FLOAT_EQ( 45.0, deg( 45.0).deg());
  EXPECT_FLOAT_EQ( 45.0, deg( 45.0L).deg());
  EXPECT_FLOAT_EQ(-45.0, deg(-45).deg());
  EXPECT_FLOAT_EQ(-45.0, deg(-45.0).deg());
  EXPECT_FLOAT_EQ(-45.0, deg(-45.0L).deg());

  EXPECT_EQ(-180,    deg(-180).deg());
  EXPECT_EQ( 179.99, deg(179.99).deg());
  EXPECT_EQ(-180,    deg(180.0).deg());
  EXPECT_EQ(-179.99, deg(180.01).deg());

  EXPECT_EQ(-179.99, deg(-179.99).deg());
  EXPECT_EQ(-180,    deg(-180.0).deg());

  EXPECT_EQ(          0, deg(0.0).deg());
  EXPECT_FLOAT_EQ(-0.01, deg(359.99).deg());
  EXPECT_FLOAT_EQ( 0.01, deg(0.01).deg());

  EXPECT_EQ_TOL(-160,    deg(200).deg(), 1E-9);
  EXPECT_EQ_TOL(  40,    deg(normalizeDeg(400)).deg(), 1E-9);

  EXPECT_EQ_TOL( 160,    deg(normalizeDeg(-200)).deg(), 1E-9);
  EXPECT_EQ_TOL( -40,    deg(normalizeDeg(-400)).deg(), 1E-9);
}

ATEST(AngleTest, Deg) {
  EXPECT_EQ( 0, deg(0.0).deg());
  EXPECT_FLOAT_EQ( 0.01, deg(0.01).deg());
  EXPECT_FLOAT_EQ( 0.18, deg(0.18).deg());
  EXPECT_FLOAT_EQ( 1.8,  deg(1.8).deg());
  EXPECT_FLOAT_EQ( 18.0, deg(18.0).deg());
  EXPECT_FLOAT_EQ( 45.0, deg( 45.0).deg());
  EXPECT_FLOAT_EQ( 179.999999, deg(179.999999).deg());
  EXPECT_FLOAT_EQ(-0.01, deg(-0.01).deg());
  EXPECT_FLOAT_EQ(-0.18, deg(-0.18).deg());
  EXPECT_FLOAT_EQ(-1.8,  deg(-1.8).deg());
  EXPECT_FLOAT_EQ(-18.0, deg(-18.0).deg());
  EXPECT_FLOAT_EQ(-45.0, deg(-45.0).deg());
  EXPECT_FLOAT_EQ(-179.999999, deg(-179.999999).deg());

  EXPECT_EQ(-180,    deg(-180.0).deg());
  EXPECT_EQ(-180,    deg(-180).deg());
  EXPECT_EQ(-179.99, deg(-179.99).deg());
  EXPECT_EQ( 179.99, deg(179.99).deg());
  EXPECT_EQ(-180,    deg(180.0).deg());
  EXPECT_EQ(-180,    deg(180).deg());
}


ATEST(AngleTest, Rad) {
  // [-M_PI, M_PI)
  const double eps = 0.01;
  EXPECT_FLOAT_EQ( M_PI - eps, rad(M_PI - eps).rad());
  EXPECT_FLOAT_EQ(-M_PI,       rad(M_PI).rad());
  EXPECT_FLOAT_EQ(-M_PI + eps, rad(M_PI + eps).rad());

  EXPECT_FLOAT_EQ(-M_PI + eps, rad(-M_PI + eps).rad());
  EXPECT_FLOAT_EQ(-M_PI,       rad(-M_PI).rad());

  EXPECT_FLOAT_EQ(0,           rad(0.0).rad());
  EXPECT_FLOAT_EQ(-eps,        rad(2 * M_PI - eps).rad());
}

ATEST(AngleTest, Add) {
  Angle a = deg(-180.00);
  Angle b = deg(179.999999999999);
  b += a;
  b.print();
  EXPECT_FLOAT_EQ(          -0.000000000001, b.deg());
  Angle c = b;
  EXPECT_FLOAT_EQ(          -0.000000000001, c.deg());
  c = a + deg(  179.999999999999);
  c.print();
  EXPECT_FLOAT_EQ(          -0.000000000001, c.deg());
  c = 0;
  EXPECT_EQ(0, c.deg());
}

ATEST(AngleTest, Add2) {
  fprintf(stderr, "Add test\n\n");

  const double eps = 1.0E-12;
  const double tol = 5.0E-18;  // 60 bits should be good for 1E-18 precision.
  // +=
  Angle x = deg(0);
  x += deg(0);
  EXPECT_EQ_TOL(0, x.deg(), tol);
  x = deg(0);
  x += deg(170);
  EXPECT_EQ_TOL(170, x.deg(), tol);
  x = deg(0);
  x += deg(-170);
  EXPECT_EQ_TOL(-170, x.deg(), tol);
  x = deg(0);
  x += deg(-180.0/256);
  EXPECT_EQ_TOL(-180.0/256, x.deg(), tol);

  // Test error accumulation. 1 iterations changes y by -1 degrees.
  Angle y;
  int fraction = 256;
  Angle almost180 = deg(180.0L - 180.0L / fraction);
  for (int n = 0; n < 10 * 2 * fraction; ++n) {
    y += almost180;
    if (n % (2 * fraction) == (2 * fraction - 1)) {
      y.print();
    }
  }
  EXPECT_EQ_TOL(0, y.deg(), 1 * 2 * fraction * tol);

  Angle a = deg(-180.00);
  Angle b = deg(180 - eps);
  b += a;
  b.print();
  EXPECT_FLOAT_EQ(-eps, b.deg());
  Angle c = b;
  EXPECT_FLOAT_EQ(-eps, c.deg());
  c = a + deg(180 - eps);
  EXPECT_FLOAT_EQ(-eps, c.deg());
}

ATEST(AngleTest, UnaryMinus) {
  Angle b = deg(179.99);
  Angle a = -b;
  EXPECT_EQ(-179.99, a.deg());
  b += -a;
  EXPECT_FLOAT_EQ(-0.02, b.deg());  // 179.99+179.99
  Angle c;
  c = -c;
  EXPECT_EQ(0, c.deg());
  c = deg(180);
  c = -c;
  EXPECT_EQ(-180.00, c.deg());
  c = deg(-180);
  c = -c;
  EXPECT_EQ(-180.00, c.deg());
  c = deg(180.00);
  c = -c;
  EXPECT_EQ(-180.00, c.deg());
  c = deg(-180.00);
  c = -c;
  EXPECT_EQ(-180.00, c.deg());
  c = deg(180.00L);
  c = -c;
  EXPECT_EQ(-180.00, c.deg());
  c = deg(-180.00L);
  c = -c;
  EXPECT_EQ(-180.00, c.deg());
}

ATEST(AngleTest, Abs) {
  Angle b = deg(179.99);
  Angle a = b.abs();
  EXPECT_EQ(179.99, a.deg());
  EXPECT_FLOAT_EQ(0.02, deg(-0.02).abs().deg());  // 179.99+179.99
  Angle c;
  c = -c;
  EXPECT_EQ(0, c.abs().deg());
  c = deg(-18);
  EXPECT_EQ(18.00, c.abs().deg());
  c = deg(-90);
  EXPECT_EQ(90.00, c.abs().deg());
  c = deg(180.00);
  EXPECT_FLOAT_EQ(180.00, c.abs().deg());
  c = deg(-180.00);
  EXPECT_FLOAT_EQ(180.00, c.abs().deg());  // actually a little bit less than 180 deg.
}

double NearestTestRad(double start, double option1, double option2, bool* took1 = NULL) {
  Angle a = rad(start);
  return a.nearest(rad(option1), rad(option2), took1).rad();
}

ATEST(AngleTest, Nearest) {
  EXPECT_FLOAT_EQ(Deg2Rad(171), NearestTestRad(Deg2Rad(179), Deg2Rad(171), Deg2Rad(-172)));
  EXPECT_FLOAT_EQ(Deg2Rad(-173), NearestTestRad(Deg2Rad(179), Deg2Rad(-173), Deg2Rad(170)));
  EXPECT_FLOAT_EQ(Deg2Rad(-173), NearestTestRad(Deg2Rad(179), Deg2Rad(170), Deg2Rad(-173)));

  EXPECT_FLOAT_EQ(1, NearestTestRad(1.2, 1, 1.5));
  EXPECT_FLOAT_EQ(1 - M_PI - 0.011, NearestTestRad(1, 1 - M_PI + 0.01, 1 + M_PI - 0.011));
  EXPECT_FLOAT_EQ(-1, NearestTestRad(-0.01, 1, -1));
  EXPECT_FLOAT_EQ( 1, NearestTestRad( 0.01, 1, -1));
  EXPECT_FLOAT_EQ( 1, NearestTestRad(M_PI - 0.01, 1, -1));
  EXPECT_FLOAT_EQ(-1, NearestTestRad(M_PI + 0.01, 1, -1));

  EXPECT_FLOAT_EQ(1, NearestTestRad(1.2, 1, 0.99));
  EXPECT_FLOAT_EQ(1 - M_PI + 0.01, NearestTestRad(1, 1 - M_PI + 0.01, 1 + M_PI - 0.009));
  EXPECT_FLOAT_EQ(-1, NearestTestRad(-0.01, -1, -1.001));
  EXPECT_FLOAT_EQ( 1, NearestTestRad( 0.01, 1, 1.001));
  EXPECT_FLOAT_EQ( 1, NearestTestRad(M_PI - 0.01, 1, 0.99));
  EXPECT_FLOAT_EQ(-1, NearestTestRad(M_PI + 0.01, -0.99, -1));
  bool left;
  EXPECT_FLOAT_EQ( 1, NearestTestRad(M_PI - 0.01, 1, 0.99, &left));
  EXPECT_TRUE(left);
  EXPECT_FLOAT_EQ( 1, NearestTestRad(1, 1, 1, &left));
  //EXPECT_TRUE(left);
  //EXPECT_FLOAT_EQ( 1.1, NearestTestRad(1, 1.1, 1.1, &left));
  //EXPECT_TRUE(left);
  EXPECT_FLOAT_EQ(-1, NearestTestRad(M_PI + 0.01, -0.99, -1, &left));
  EXPECT_FALSE(left);
}


ATEST(AngleTest, Opposite) {
  Angle c;
  c = c.opposite();
  EXPECT_EQ(-180, c.deg());
  Angle d = deg(179.99);
  EXPECT_FLOAT_EQ(-0.01, d.opposite().deg());
}

ATEST(AngleTest, Div) {
  Angle c;
  c = c / 2;
  EXPECT_EQ(0, c.deg());
  Angle d = deg(179.99);
  c = d / 2;
  EXPECT_FLOAT_EQ(89.995, c.deg());
  d = deg(-179.99);
  c = d / 2;
  EXPECT_FLOAT_EQ(-89.995, c.deg());
}

ATEST(AngleTest, Mul) {
  Angle c;
  c = c * 0.2;
  EXPECT_EQ(0, c.deg());
  c = deg(-70);
  c = c * 0.142856142856;
  EXPECT_FLOAT_EQ(-10, c.deg());
  Angle d = deg(179.99);
  c = d * 0.5;
  EXPECT_FLOAT_EQ(89.995, c.deg());
  d = deg(-179.99);
  c = d * 0.5;
  EXPECT_FLOAT_EQ(-89.995, c.deg());
}

ATEST(AngleTest, Trigo) {
  Angle c;
  for (int n = 0; n < 360; ++n) {
    c += deg(1);
    double cx = c.cos();
    double cy = c.sin();
    EXPECT_FLOAT_EQ(1, cx * cx + cy * cy);
    Angle reproduced = fromAtan2(cy, cx);
    EXPECT_FLOAT_EQ(0, (c - reproduced).deg());
  }
  c = fromAtan2(0, -1);
  EXPECT_FLOAT_EQ(-180, c.deg());
  c = fromAtan2(1, -1);
  EXPECT_FLOAT_EQ(135, c.deg());
  c = fromAtan2(1, 0);
  EXPECT_FLOAT_EQ(90, c.deg());
  c = fromAtan2(1, 1);
  EXPECT_FLOAT_EQ(45, c.deg());
  c = fromAtan2(0, 0);
  EXPECT_FLOAT_EQ(0, c.deg());
  c = fromAtan2(-1, -1);
  EXPECT_FLOAT_EQ(-135, c.deg());
}

ATEST(AngleTest, SignETC) {
  Angle b = deg(179.99);
  Angle a = -b;
  EXPECT_EQ(-1, a.sign());
  EXPECT_EQ(-1, a.signNotZero());
  EXPECT_EQ(false, a.positive());
  EXPECT_EQ(true, a.negative());

  EXPECT_EQ(1, b.sign());
  EXPECT_EQ(1, b.signNotZero());
  EXPECT_EQ(true, b.positive());
  EXPECT_EQ(false, b.negative());

  Angle c;
  EXPECT_EQ(0, c.sign());
  EXPECT_EQ(1, c.signNotZero());
  EXPECT_EQ(false, c.positive());
  EXPECT_EQ(false, c.negative());

  Angle small = deg(179.99);
  Angle big = -small;
  EXPECT_EQ(false, big < small);
  EXPECT_EQ(true, big > small);
  EXPECT_EQ(true, big < c);
  EXPECT_EQ(false, big > c);
  EXPECT_EQ(false, small < c);
  EXPECT_EQ(true, small > c);

  small = deg(180);
  big = deg(-179.99);
  EXPECT_EQ(false, big < small);
  EXPECT_EQ(true, big > small);
  EXPECT_EQ(true, big < c);
  EXPECT_EQ(false, big > c);
  EXPECT_EQ(false, small < c);  // -180deg < 0 (def. ==) false
  EXPECT_EQ(false, small > c);  // Yes, this looks unusual but makes sense
                                // because 0 > 0 is false as 0 < 0.
  small = deg(179.99);
  big = deg(180);
  EXPECT_EQ(false, big < small);
  EXPECT_EQ(true, big > small);
  EXPECT_EQ(false, big < c);
  EXPECT_EQ(false, big > c);  // See comment above.
  EXPECT_EQ(false, small < c);
  EXPECT_EQ(true, small > c);

  for (double diff = 0.1; diff < 180; diff+= 0.1) {
    for (double x = 0; x < 360; x += 1.0) {
      // printf("x=%lf diff=%lf\n", x, diff);
      EXPECT_TRUE(deg(x) < deg(x) + deg(diff));
      EXPECT_TRUE(deg(x) <= deg(x) + deg(diff));
      EXPECT_TRUE(deg(x) == deg(x) + deg(0.0));
      EXPECT_TRUE(deg(x) != deg(x) + deg(diff));
      EXPECT_FALSE(deg(x) > deg(x) + deg(diff));
      EXPECT_FALSE(deg(x) >= deg(x) + deg(diff));
      EXPECT_FALSE(deg(x) != deg(x) + deg(0.0));
      EXPECT_FALSE(deg(x) == deg(x) + deg(diff));
    }
  }


}


int main(int argc, char* argv[]) {
  return testing::RunAllTests();
}
