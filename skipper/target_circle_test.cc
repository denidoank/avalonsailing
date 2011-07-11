// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "skipper/target_circle.h"
#include "lib/testing/testing.h"


TEST(TargetCircle, All) {
  TargetCircle t(0, 0, 1);
  EXPECT_FLOAT_EQ(0, t.ToDeg(0, 0));
  EXPECT_FLOAT_EQ(0, t.ToDeg(-2, 0));

  EXPECT_FLOAT_EQ(   0, t.ToDeg(-1,  0));
  EXPECT_FLOAT_EQ(  45, t.ToDeg(-1, -1));
  EXPECT_FLOAT_EQ(  90, t.ToDeg( 0, -1));
  EXPECT_FLOAT_EQ( 135, t.ToDeg( 1, -1));
  EXPECT_FLOAT_EQ( 180, t.ToDeg( 1,  0));
  EXPECT_FLOAT_EQ( 225, t.ToDeg( 1,  1));
  EXPECT_FLOAT_EQ( 270, t.ToDeg( 0,  1));
  EXPECT_FLOAT_EQ( 315, t.ToDeg(-1,  1));

  EXPECT_FLOAT_EQ( 225, t.ToDeg( 10,  10));
  EXPECT_FLOAT_EQ(sqrt(2) * 10, t.Distance(10, 10));
  EXPECT_EQ(false, t.In(1.01, 0));
  EXPECT_EQ(true,  t.In(0.99, 0));
  EXPECT_EQ(false, t.In(2.01, 0, 2));  // expansion of the radius by a factor
  EXPECT_EQ(true,  t.In(1.99, 0, 2));
  }


  TEST(TargetCircle, Real) {
  TargetCircle t(100, 100, 1);
  EXPECT_FLOAT_EQ(45, t.ToDeg(0, 0));
  EXPECT_FLOAT_EQ(44.4327, t.ToDeg(-2, 0));

  EXPECT_FLOAT_EQ(   0, t.ToDeg(100 + -1, 100 +  0));
  EXPECT_FLOAT_EQ(  45, t.ToDeg(100 + -1, 100 + -1));
  EXPECT_FLOAT_EQ(  90, t.ToDeg(100 +  0, 100 + -1));
  EXPECT_FLOAT_EQ( 135, t.ToDeg(100 +  1, 100 + -1));
  EXPECT_FLOAT_EQ( 180, t.ToDeg(100 +  1, 100 +  0));
  EXPECT_FLOAT_EQ( 225, t.ToDeg(100 +  1, 100 +  1));
  EXPECT_FLOAT_EQ( 270, t.ToDeg(100 +  0, 100 +  1));
  EXPECT_FLOAT_EQ( 315, t.ToDeg(100 + -1, 100 +  1));

  EXPECT_FLOAT_EQ( 45, t.ToDeg( 10,  10));
  EXPECT_FLOAT_EQ(sqrt(2) * 100, t.Distance(0, 0));
  EXPECT_EQ(false, t.In(1.01, 0));
  EXPECT_EQ(false,  t.In(0.99, 0));
  EXPECT_EQ(false, t.In(2.01, 0, 2));  // expansion of the radius by a factor
  EXPECT_EQ(false,  t.In(1.99, 0, 2));
  EXPECT_EQ(true,  t.In(101.99, 100, 2));
  EXPECT_EQ(true,  t.In(99.4, 100.6));

  EXPECT_FLOAT_EQ( 0, t.ToDeg( 100,  100));
}


int main(int argc, char* argv[]) {
  TargetCircle_All();
  TargetCircle_Real();
  return 0;
}
