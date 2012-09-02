// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
//
// Unit test for common/normalize
#include "common/delta_angle.h"

#include <math.h>
#include "lib/testing/testing.h"


TEST(DeltaAngle, All) {
  EXPECT_TRUE(Less(0, 1));
  EXPECT_TRUE(Less(3, 4));
  EXPECT_TRUE(Less(3, -3));
  EXPECT_FALSE(Less(0, -0.0001));
  EXPECT_FALSE(Less(-3.14, 3.14));
  EXPECT_FALSE(Less(-3, -3));
  EXPECT_TRUE(LessOrEqual(0, 1));
  EXPECT_TRUE(LessOrEqual(3, 4));
  EXPECT_TRUE(LessOrEqual(3, -3));
  EXPECT_TRUE(LessOrEqual(3, 3));
  EXPECT_TRUE(LessOrEqual(-3, -3));
  EXPECT_TRUE(LessOrEqual(-M_PI, M_PI));
  EXPECT_FALSE(LessOrEqual(-3.14, 3.14));
  EXPECT_TRUE(LessOrEqual(-3, -3));

  EXPECT_FLOAT_EQ(-0.1, Minimum(-0.1, 0.1));
  EXPECT_FLOAT_EQ(-0.9, Minimum(-0.9, 0.9));
  EXPECT_FLOAT_EQ(-1.5, Minimum(-1.5, 1.5));
  EXPECT_FLOAT_EQ( 1.6, Minimum(-1.6, 1.6));
  EXPECT_FLOAT_EQ( 3.0, Minimum(-3.0, 3.0));
  EXPECT_FLOAT_EQ( 3.14, Minimum(-3.14, 3.14));
  EXPECT_FLOAT_EQ( 3.14-2*M_PI, Minimum(-3.14+2*M_PI, 3.14-2*M_PI));

  EXPECT_FLOAT_EQ( 0.1, Maximum(-0.1, 0.1));
  EXPECT_FLOAT_EQ( 0.9, Maximum(-0.9, 0.9));
  EXPECT_FLOAT_EQ( 1.5, Maximum(-1.5, 1.5));
  EXPECT_FLOAT_EQ(-1.6, Maximum(-1.6, 1.6));
  EXPECT_FLOAT_EQ(-3.0, Maximum(-3.0, 3.0));
  EXPECT_FLOAT_EQ(-3.14, Maximum(-3.14, 3.14));
  EXPECT_FLOAT_EQ(-3.14+2*M_PI, Maximum(-3.14+2*M_PI, 3.14));
  EXPECT_FLOAT_EQ(-3.14+2*M_PI, Maximum(-3.14+2*M_PI, 3.14+2*M_PI));
  EXPECT_FLOAT_EQ(-3.14-2*M_PI, Maximum(-3.14-2*M_PI, 3.14+4*M_PI));

  EXPECT_EQ(-M_PI, DeltaOldNewRad(M_PI, 0));
  EXPECT_EQ(-M_PI, DeltaOldNewRad(0, M_PI));
  EXPECT_EQ(0, DeltaOldNewRad(0, 0));
  EXPECT_FLOAT_EQ(0.1, DeltaOldNewRad(0, 0.1));
  EXPECT_FLOAT_EQ(-0.1, DeltaOldNewRad(0.1, 0));
  EXPECT_FLOAT_EQ(-M_PI / 4, DeltaOldNewRad(M_PI, 3 * M_PI / 4));

  EXPECT_FLOAT_EQ(170, NearerDeg(179, 170, -172));
  EXPECT_FLOAT_EQ(-172, NearerDeg(179, -172, 170));
  EXPECT_FLOAT_EQ(-173, NearerDeg(179, 170, -173));

  EXPECT_FLOAT_EQ(1, NearerRad(1.2, 1, 1.5));
  EXPECT_FLOAT_EQ(1 + M_PI - 0.011, NearerRad(1, 1 - M_PI + 0.01, 1 + M_PI - 0.011));
  EXPECT_FLOAT_EQ(-1, NearerRad(-0.01, 1, -1));
  EXPECT_FLOAT_EQ( 1, NearerRad( 0.01, 1, -1));
  EXPECT_FLOAT_EQ( 1, NearerRad(M_PI - 0.01, 1, -1));
  EXPECT_FLOAT_EQ(-1, NearerRad(M_PI + 0.01, 1, -1));

  EXPECT_FLOAT_EQ(1, NearerRad(1.2, 1, 0.99));
  EXPECT_FLOAT_EQ(1 - M_PI + 0.01, NearerRad(1, 1 - M_PI + 0.01, 1 + M_PI - 0.009));
  EXPECT_FLOAT_EQ(-1, NearerRad(-0.01, -1, -1.001));
  EXPECT_FLOAT_EQ( 1, NearerRad( 0.01, 1, 1.001));
  EXPECT_FLOAT_EQ( 1, NearerRad(M_PI - 0.01, 1, 0.99));
  EXPECT_FLOAT_EQ(-1, NearerRad(M_PI + 0.01, -0.99, -1));
  bool left;
  EXPECT_FLOAT_EQ( 1, NearerRad(M_PI - 0.01, 1, 0.99, &left));
  EXPECT_TRUE(left);
  EXPECT_FLOAT_EQ(-1, NearerRad(M_PI + 0.01, -0.99, -1, &left));
  EXPECT_FALSE(left);
}

int main(int argc, char* argv[]) {
  DeltaAngle_All();
  return 0;
}
