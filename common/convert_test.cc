// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Unit test for common/convert
#include "common/convert.h"

#include <math.h>

#include "common/testing.h"

TEST(CommonTransformTest, All) {
// Angle
EXPECT_EQ(-M_PI, Deg2Rad(-180));
EXPECT_EQ(-1, Deg2Rad(-180 / M_PI));
EXPECT_EQ(0, Deg2Rad(0));
EXPECT_EQ(1, Deg2Rad(180 / M_PI));
EXPECT_EQ(M_PI / 2.0, Deg2Rad(90));
EXPECT_EQ(M_PI, Deg2Rad(180));
EXPECT_EQ(2 * M_PI, Deg2Rad(360));
EXPECT_EQ(4 * M_PI, Deg2Rad(720));

EXPECT_EQ(-180, Rad2Deg(-M_PI));
EXPECT_EQ(90, Rad2Deg(M_PI / 2));

// Distance
EXPECT_EQ(1852.0, NauticalMileToMeter(1));
EXPECT_EQ(-10, MeterToNauticalMile(-18520));

// Speed
EXPECT_FLOAT_EQ(18.52 / 3.6, KnotsToMeterPerSecond(10));
EXPECT_FLOAT_EQ(-10, MeterPerSecondToKnots(-18.52 / 3.6));
}

int main(int argc, char* argv[]) {
  CommonTransformTest_All();
  return 0;
}
