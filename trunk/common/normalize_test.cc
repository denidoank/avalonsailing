// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
//
// Unit test for common/normalize
#include "common/normalize.h"
#include "common/testing.h"

#include <math.h>


TEST(CommonNormalizeTest, AllNormalizeAngle) {
EXPECT_EQ(179.99, NormalizeAngle(179.99));
EXPECT_EQ(180,    NormalizeAngle(180));
EXPECT_EQ(180.01, NormalizeAngle(180.01));

EXPECT_EQ(180.01, NormalizeAngle(-179.99));
EXPECT_EQ(180,    NormalizeAngle(-180));
EXPECT_EQ(179.99, NormalizeAngle(-180.01));

EXPECT_EQ(0,      NormalizeAngle(0));
EXPECT_EQ(359.99, NormalizeAngle(359.99));
EXPECT_FLOAT_EQ(0.01, NormalizeAngle(360.01));

EXPECT_EQ(0, NormalizeAngle(-10800000));
EXPECT_EQ(0, NormalizeAngle(-108000));
EXPECT_EQ(0, NormalizeAngle(-1080));
EXPECT_EQ(0, NormalizeAngle(-720));
EXPECT_EQ(0, NormalizeAngle(-360));
EXPECT_EQ(0, NormalizeAngle(0));
EXPECT_EQ(0, NormalizeAngle(360));
EXPECT_EQ(0, NormalizeAngle(720));
EXPECT_EQ(0, NormalizeAngle(1080));
EXPECT_EQ(0, NormalizeAngle(108000));
EXPECT_EQ(0, NormalizeAngle(10800000));

EXPECT_EQ(359, NormalizeAngle(-10800001));
EXPECT_EQ(1,   NormalizeAngle( 10800001));
}

TEST(CommonNormalizeTest, SymmetricAngle) {
// (-180, 180] !!!
EXPECT_EQ( 179.99, SymmetricAngle(179.99));
EXPECT_EQ( 180,    SymmetricAngle(180));
EXPECT_EQ(-179.99, SymmetricAngle(180.01));

EXPECT_EQ(-179.99, SymmetricAngle(-179.99));
EXPECT_EQ(-180,    SymmetricAngle(-180));
EXPECT_EQ( 179.99, SymmetricAngle(-180.01));

EXPECT_EQ(0, SymmetricAngle(-10800000));
EXPECT_EQ(0, SymmetricAngle(-108000));
EXPECT_EQ(0, SymmetricAngle(-1080));
EXPECT_EQ(0, SymmetricAngle(-720));
EXPECT_EQ(0, SymmetricAngle(-360));
EXPECT_EQ(0, SymmetricAngle(0));
EXPECT_EQ(0, SymmetricAngle(360));
EXPECT_EQ(0, SymmetricAngle(720));
EXPECT_EQ(0, SymmetricAngle(1080));
EXPECT_EQ(0, SymmetricAngle(108000));
EXPECT_EQ(0, SymmetricAngle(10800000));

EXPECT_EQ(-1, SymmetricAngle(-10800001));
EXPECT_EQ( 1, SymmetricAngle( 10800001));
}

int main(int argc, char* argv[]) {
  CommonNormalizeTest_AllNormalizeAngle();
  CommonNormalizeTest_SymmetricAngle();
  return 0;
}
