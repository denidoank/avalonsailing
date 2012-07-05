// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
//
// Unit test for common/normalize
#include "common/normalize.h"

#include <math.h>
#include "lib/testing/testing.h"


TEST(CommonNormalizeTest, AllNormalizeDeg) {
EXPECT_EQ(179.99, NormalizeDeg(179.99));
EXPECT_EQ(180,    NormalizeDeg(180));
EXPECT_EQ(180.01, NormalizeDeg(180.01));

EXPECT_EQ(180.01, NormalizeDeg(-179.99));
EXPECT_EQ(180,    NormalizeDeg(-180));
EXPECT_EQ(179.99, NormalizeDeg(-180.01));

EXPECT_EQ(0,      NormalizeDeg(0));
EXPECT_EQ(359.99, NormalizeDeg(359.99));
EXPECT_FLOAT_EQ(0.01, NormalizeDeg(360.01));

EXPECT_EQ(0, NormalizeDeg(-10800000));
EXPECT_EQ(0, NormalizeDeg(-108000));
EXPECT_EQ(0, NormalizeDeg(-1080));
EXPECT_EQ(0, NormalizeDeg(-720));
EXPECT_EQ(0, NormalizeDeg(-360));
EXPECT_EQ(0, NormalizeDeg(0));
EXPECT_EQ(0, NormalizeDeg(360));
EXPECT_EQ(0, NormalizeDeg(720));
EXPECT_EQ(0, NormalizeDeg(1080));
EXPECT_EQ(0, NormalizeDeg(108000));
EXPECT_EQ(0, NormalizeDeg(10800000));

EXPECT_EQ(359, NormalizeDeg(-10800001));
EXPECT_EQ(1,   NormalizeDeg( 10800001));
}

TEST(CommonNormalizeTest, SymmetricDeg) {
// [-180, 180)
EXPECT_EQ( 179.99, SymmetricDeg(179.99));
EXPECT_EQ(-180,    SymmetricDeg(180));
EXPECT_EQ(-179.99, SymmetricDeg(180.01));

EXPECT_EQ(-179.99, SymmetricDeg(-179.99));
EXPECT_EQ(-180,    SymmetricDeg(-180));
EXPECT_EQ( 179.99, SymmetricDeg(-180.01));

// [-180, 180)
EXPECT_EQ(-160,    SymmetricDeg(200));
EXPECT_EQ(  40,    SymmetricDeg(400));

EXPECT_EQ( 160,    SymmetricDeg(-200));
EXPECT_EQ( -40,    SymmetricDeg(-400));


EXPECT_EQ(0, SymmetricDeg(-10800000));
EXPECT_EQ(0, SymmetricDeg(-108000));
EXPECT_EQ(0, SymmetricDeg(-1080));
EXPECT_EQ(0, SymmetricDeg(-720));
EXPECT_EQ(0, SymmetricDeg(-360));
EXPECT_EQ(0, SymmetricDeg(0));
EXPECT_EQ(0, SymmetricDeg(360));
EXPECT_EQ(0, SymmetricDeg(720));
EXPECT_EQ(0, SymmetricDeg(1080));
EXPECT_EQ(0, SymmetricDeg(108000));
EXPECT_EQ(0, SymmetricDeg(10800000));

EXPECT_EQ(-1, SymmetricDeg(-10800001));
EXPECT_EQ( 1, SymmetricDeg( 10800001));
}

TEST(CommonNormalizeTest, AllNormalizeRad) {
const double eps = 0.01;
EXPECT_FLOAT_EQ(M_PI - eps, NormalizeRad(M_PI - eps));
EXPECT_FLOAT_EQ(M_PI,       NormalizeRad(M_PI));
EXPECT_FLOAT_EQ(M_PI + eps, NormalizeRad(M_PI + eps));

EXPECT_FLOAT_EQ(M_PI + eps, NormalizeRad(-M_PI + eps));
EXPECT_FLOAT_EQ(M_PI,       NormalizeRad(-M_PI));
EXPECT_FLOAT_EQ(M_PI - eps, NormalizeRad(-M_PI - eps));

EXPECT_FLOAT_EQ(0,              NormalizeRad(0));
EXPECT_FLOAT_EQ(2 * M_PI - eps, NormalizeRad(2 * M_PI - eps));
EXPECT_FLOAT_EQ(eps, NormalizeRad(2 * M_PI + eps));

EXPECT_FLOAT_EQ(0, NormalizeRad(-300 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, NormalizeRad( -30 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, NormalizeRad(  -3 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, NormalizeRad(  -2 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, NormalizeRad(  -1 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, NormalizeRad(   0 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, NormalizeRad(   1 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, NormalizeRad(   2 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, NormalizeRad(   3 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, NormalizeRad(  30 * 2 * M_PI + 1E-6));
EXPECT_FLOAT_EQ(0, NormalizeRad( 300 * 2 * M_PI + 1E-6));

EXPECT_FLOAT_EQ(2 * M_PI - eps, NormalizeRad(-300 * 2 * M_PI - eps));
EXPECT_FLOAT_EQ(           eps, NormalizeRad( 300 * 2 * M_PI + eps));
}

TEST(CommonNormalizeTest, SymmetricRad) {
const double eps = 0.01;
// [-M_PI, M_PI)
EXPECT_FLOAT_EQ( M_PI - eps, SymmetricRad(M_PI - eps));
EXPECT_FLOAT_EQ(-M_PI,       SymmetricRad(M_PI));
EXPECT_FLOAT_EQ(-M_PI + eps, SymmetricRad(M_PI + eps));

EXPECT_FLOAT_EQ(-M_PI + eps, SymmetricRad(-M_PI + eps));
EXPECT_FLOAT_EQ(-M_PI,    SymmetricRad(-M_PI));
EXPECT_FLOAT_EQ( M_PI - eps, SymmetricRad(-M_PI - eps));

EXPECT_FLOAT_EQ(0, SymmetricRad(-300 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, SymmetricRad( -30 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, SymmetricRad(  -3 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, SymmetricRad(  -2 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, SymmetricRad(  -1 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, SymmetricRad(   0 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, SymmetricRad(   1 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, SymmetricRad(   2 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, SymmetricRad(   3 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, SymmetricRad(  30 * 2 * M_PI));
EXPECT_FLOAT_EQ(0, SymmetricRad( 300 * 2 * M_PI));

EXPECT_FLOAT_EQ(-eps, SymmetricRad(-300 * 2 * M_PI - eps));
EXPECT_FLOAT_EQ( eps, SymmetricRad( 300 * 2 * M_PI + eps));
}


int main(int argc, char* argv[]) {
  CommonNormalizeTest_AllNormalizeDeg();
  CommonNormalizeTest_SymmetricDeg();
  CommonNormalizeTest_AllNormalizeRad();
  CommonNormalizeTest_SymmetricRad();
  return 0;
}
