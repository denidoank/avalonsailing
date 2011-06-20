// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "common/polar.h"

#include <math.h>
#include "lib/testing/testing.h"

TEST(Polar, All) {
  Polar a(0, 1);
  Polar b(0, 2);
  Polar c = a + b;
  EXPECT_EQ(0, c.AngleRad());
  EXPECT_EQ(3, c.Mag());
  
  Polar d = a - b;
  EXPECT_EQ(M_PI, d.AngleRad());
  EXPECT_EQ(1, d.Mag());
  EXPECT_EQ(0, a.AngleRad());
  EXPECT_EQ(1, a.Mag());

  Polar e = a + Polar(M_PI / 2 , 1);
  EXPECT_FLOAT_EQ(M_PI / 4, e.AngleRad());
  EXPECT_FLOAT_EQ(sqrt(2), e.Mag());
  EXPECT_EQ(0, a.AngleRad());
  EXPECT_EQ(1, a.Mag());
  
  e = a - Polar(M_PI / 2 , 1);
  EXPECT_FLOAT_EQ(-M_PI / 4, e.AngleRad());
  EXPECT_FLOAT_EQ(sqrt(2), e.Mag());
  EXPECT_EQ(0, a.AngleRad());
  EXPECT_EQ(1, a.Mag());
}

int main(int argc, char* argv[]) {
  Polar_All();
  return 0;
}
