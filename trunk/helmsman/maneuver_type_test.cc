// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/maneuver_type.h"

#include <string.h>
#include "common/convert.h"
#include "lib/testing/testing.h"


TEST(FindManeuverType, All) {
  double true_wind = -M_PI;  // So we see the usual sailing diagram
                             // wind coming from top.
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(-1), Deg2Rad( 1), true_wind));
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad( 1), Deg2Rad(-1), true_wind));

  EXPECT_EQ(kChange, FindManeuverType(Deg2Rad( 1), Deg2Rad( 2), true_wind));
  EXPECT_EQ(kChange, FindManeuverType(Deg2Rad(-1), Deg2Rad(-2), true_wind));

  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(90), Deg2Rad(-89), true_wind));
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(90), Deg2Rad(-91), true_wind));
  
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(-90), Deg2Rad(89), true_wind));
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(-90), Deg2Rad(91), true_wind));
  
  // limit cases with 180 degrees turn, either way is fine with me.
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(90), Deg2Rad(-90), true_wind));
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(89), Deg2Rad(-91), true_wind));
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(-90), Deg2Rad(90), true_wind));
  EXPECT_EQ(kTack,   FindManeuverType(Deg2Rad(95), Deg2Rad(-85), true_wind));
  
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(175), Deg2Rad(-175), true_wind));
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(130), Deg2Rad(-130), true_wind));
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(100), Deg2Rad(-100), true_wind));

  true_wind = Deg2Rad(-179);
  EXPECT_EQ(kJibe,   FindManeuverType(Deg2Rad(-100), Deg2Rad(90), true_wind));

  EXPECT_EQ(0, strcmp("Change", ManeuverToString(kChange)));
  EXPECT_EQ(0, strcmp("Jibe",   ManeuverToString(kJibe)));
  EXPECT_EQ(0, strcmp("Tack",   ManeuverToString(kTack)));
}


int main(int argc, char* argv[]) {
  FindManeuverType_All();
  return 0;
}
