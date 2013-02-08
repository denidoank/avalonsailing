// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "common/polar.h"

#include <math.h>
#include "common/angle.h"
#include "lib/testing/testing.h"

TEST(Polar, All) {
  Polar x(0, 1);
  EXPECT_EQ(0, x.AngleRad());
  EXPECT_ANGLE_EQ(rad(0), x.Arg());
  EXPECT_EQ(1, x.Mag());

  Polar y(2, 0);
  EXPECT_EQ(0, y.AngleRad());
  EXPECT_ANGLE_EQ(rad(0), y.Arg());
  EXPECT_EQ(0, y.Mag());

  Polar z(0, -1);
  EXPECT_EQ(0, z.AngleRad());
  EXPECT_ANGLE_EQ(rad(0), z.Arg());
  EXPECT_EQ(-1, z.Mag());

  Polar a(0, 1);
  Polar b(0, 2);
  Polar c = a + b;
  EXPECT_EQ(0, c.AngleRad());
  EXPECT_ANGLE_EQ(rad(0), c.Arg());
  EXPECT_EQ(3, c.Mag());
  
  Polar d = a - b;
  EXPECT_FLOAT_EQ(-M_PI, d.AngleRad());
  EXPECT_EQ(1, d.Mag());
  EXPECT_EQ(0, a.AngleRad());
  EXPECT_ANGLE_EQ(rad(0), a.Arg());
  EXPECT_EQ(1, a.Mag());

  Polar e = a + Polar(M_PI / 2 , 1);
  EXPECT_FLOAT_EQ(M_PI / 4, e.AngleRad());
  EXPECT_ANGLE_EQ(rad(M_PI / 4), e.Arg());
  EXPECT_FLOAT_EQ(sqrt(2), e.Mag());
  EXPECT_EQ(0, a.AngleRad());
  EXPECT_ANGLE_EQ(rad(0), a.Arg());
  EXPECT_EQ(1, a.Mag());
  
  e = a - Polar(M_PI / 2 , 1);
  EXPECT_FLOAT_EQ(-M_PI / 4, e.AngleRad());
  EXPECT_ANGLE_EQ(rad(-M_PI / 4), e.Arg());
  EXPECT_FLOAT_EQ(sqrt(2), e.Mag());
  EXPECT_EQ(0, a.AngleRad());
  EXPECT_EQ(1, a.Mag());

  e = Polar(M_PI / 2 ,  1) + Polar(3 * M_PI / 2 ,  1) +
      Polar(M_PI / 2 , -1) + Polar(3 * M_PI / 2 , -1);
  EXPECT_FLOAT_EQ(0, e.AngleRad());
  EXPECT_ANGLE_EQ(rad(0), e.Arg());
  EXPECT_FLOAT_EQ(0, e.Mag());

  Polar zero(0, 0);
  x = Polar(0, 1);
  Polar rev_x = zero - x;
  EXPECT_ANGLE_EQ(deg(-180), rev_x.Arg());
  EXPECT_FLOAT_EQ(1, rev_x.Mag());
  rev_x = zero - Polar(deg(1), 2);
  EXPECT_ANGLE_EQ(deg(-179), rev_x.Arg());
  EXPECT_FLOAT_EQ(2, rev_x.Mag());
  rev_x = zero - Polar(deg(90), 2);
  EXPECT_ANGLE_EQ(deg(-90), rev_x   .Arg());
  EXPECT_FLOAT_EQ(2, rev_x.Mag());

}

TEST(Polar, Mirror) {
  Polar up(0, 1);
  Polar right(M_PI / 2, 1);
  Polar up_right = up + right;
  EXPECT_EQ(rad(0), up.MirrorOnXAxis().Arg());
  EXPECT_EQ(rad(-M_PI / 2), right.MirrorOnXAxis().Arg());
  EXPECT_EQ(rad(-M_PI / 4), up_right.MirrorOnXAxis().Arg());
  EXPECT_EQ(1, up.MirrorOnXAxis().Mag());
  EXPECT_EQ(1, right.MirrorOnXAxis().Mag());
  EXPECT_EQ(sqrt(2), up_right.MirrorOnXAxis().Mag());

  EXPECT_EQ(rad(-M_PI), up.MirrorOnYAxis().Arg());
  EXPECT_EQ(rad(M_PI / 2), right.MirrorOnYAxis().Arg());
  EXPECT_EQ(rad(3 * M_PI / 4), up_right.MirrorOnYAxis().Arg());
  EXPECT_EQ(1, up.MirrorOnYAxis().Mag());
  EXPECT_EQ(1, right.MirrorOnYAxis().Mag());
  EXPECT_EQ(sqrt(2), up_right.MirrorOnYAxis().Mag());

}

int main(int argc, char* argv[]) {
  Polar_All();
  Polar_Mirror();
  return 0;
}
