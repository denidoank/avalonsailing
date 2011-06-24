// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/skipper_input.h"

#include "lib/testing/testing.h"

TEST(SkipperInput, All) {
  SkipperInput s("longitude_deg:10 latitude_deg:11.1 "
                 "angle_true_deg:-33 mag_true_kn:4.\n");
  EXPECT_EQ("longitude_deg:10.000000 latitude_deg:11.100000 "
            "angle_true_deg:-33.000000 mag_true_kn:4.000000\n", s.ToString());
  EXPECT_EQ(10, s.longitude_deg);
  EXPECT_EQ(11.1, s.latitude_deg);
  EXPECT_EQ(-33, s.angle_true_deg);
  EXPECT_EQ(4, s.mag_true_kn);

  s.Reset();
  EXPECT_EQ(0, s.longitude_deg);
  EXPECT_EQ(0, s.latitude_deg);
  EXPECT_EQ(0, s.angle_true_deg);
  EXPECT_EQ(0, s.mag_true_kn);
}

int main() {
  SkipperInput_All();
  return 0;
}
