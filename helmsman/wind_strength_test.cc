// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "helmsman/wind_strength.h"

#include "lib/testing/testing.h"


TEST(WindStrength, Hysteresis)  {
  WindStrengthRange w = kCalmWind;
  w = WindStrength(w, 0.1);
  EXPECT_EQ(kCalmWind, w);

  w = WindStrength(w, 100);
  EXPECT_EQ(kNormalWind, w);
  w = WindStrength(w, 100);
  EXPECT_EQ(kStormWind, w);

  w = WindStrength(w, 0.1);
  EXPECT_EQ(kNormalWind, w);
  w = WindStrength(w, 0.1);
  EXPECT_EQ(kCalmWind, w);

  w = WindStrength(w, 0.5);
  EXPECT_EQ(kCalmWind, w);
  w = WindStrength(w, 0.7);
  EXPECT_EQ(kNormalWind, w);

  w = WindStrength(w, 20);
  EXPECT_EQ(kNormalWind, w);
  w = WindStrength(w, 20);
  EXPECT_EQ(kNormalWind, w);
  w = WindStrength(w, 24.5);
  EXPECT_EQ(kStormWind, w);

  w = WindStrength(w, 20);
  EXPECT_EQ(kStormWind, w);
  w = WindStrength(w, 20);
  EXPECT_EQ(kStormWind, w);
  w = WindStrength(w, 15);
  EXPECT_EQ(kNormalWind, w);

  w = WindStrength(w, 0.1);
  EXPECT_EQ(kCalmWind, w);
  w = WindStrength(w, 0.1);
  EXPECT_EQ(kCalmWind, w);
}  

int main(int argc, char* argv[]) {
  WindStrength_Hysteresis();
  return 0;
} 
