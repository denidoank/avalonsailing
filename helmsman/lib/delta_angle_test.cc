// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
//
// Unit test for common/normalize
#include "delta_angle.h"

#include <math.h>
#include "lib/testing/testing.h"


TEST(DeltaAngle, All) {
  EXPECT_EQ(-M_PI, DeltaOldNewRad(M_PI, 0));
  EXPECT_EQ(-M_PI, DeltaOldNewRad(0, M_PI));
  EXPECT_EQ(0, DeltaOldNewRad(0, 0));
  EXPECT_FLOAT_EQ(0.1, DeltaOldNewRad(0, 0.1));
  EXPECT_FLOAT_EQ(-0.1, DeltaOldNewRad(0.1, 0));
  EXPECT_FLOAT_EQ(-M_PI / 4, DeltaOldNewRad(M_PI, 3 * M_PI / 4));

  EXPECT_FLOAT_EQ(170, NearerDeg(179, 170, -172));
  EXPECT_FLOAT_EQ(-172, NearerDeg(179, -172, 170));
  EXPECT_FLOAT_EQ(-173, NearerDeg(179, 170, -173));
}

int main(int argc, char* argv[]) {
  DeltaAngle_All();
  return 0;
}
