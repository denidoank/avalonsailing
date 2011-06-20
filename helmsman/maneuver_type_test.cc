// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/maneuver_type.h"

#include "common/convert.h"
#include "lib/testing/testing.h"

TEST(FindManeuverType, All) {
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad( 1), Deg2Rad(-1)));
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(-1), Deg2Rad( 1)));

  EXPECT_EQ(kChange, FindManeuverType(Deg2Rad( 1), Deg2Rad( 2)));
  EXPECT_EQ(kChange, FindManeuverType(Deg2Rad(-1), Deg2Rad(-2)));

  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(90), Deg2Rad(-89)));
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(90), Deg2Rad(-90)));
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(90), Deg2Rad(-91)));
  
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(-90), Deg2Rad(89)));
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(-90), Deg2Rad(90)));
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(-90), Deg2Rad(91)));
  
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(89), Deg2Rad(-91)));
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(95), Deg2Rad(-85)));
  
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(175), Deg2Rad(-175)));
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(130), Deg2Rad(-130)));
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(100), Deg2Rad(-100)));
}

int main(int argc, char* argv[]) {
  FindManeuverType_All();
  return 0;
}
