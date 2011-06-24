// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011


#include "helmsman/apparent.h"

#include "common/convert.h"
#include "lib/testing/testing.h"


TEST(Apparent, All) {
  Polar true_wind(0, 10);
  Polar motion(0.1, -2);
  double angle;
  double mag;
  Apparent(true_wind.AngleRad(), true_wind.Mag(),
           motion.AngleRad(), motion.Mag(),
           &angle, &mag);
  EXPECT_FLOAT_EQ(-0.0833488, angle);
  EXPECT_FLOAT_EQ(11.9917, mag);  
 
  Polar app(0, 0);
  ApparentPolar(true_wind, motion, &app);
  EXPECT_FLOAT_EQ(-0.0833488, app.AngleRad());
  EXPECT_FLOAT_EQ(11.9917, app.Mag());
}

int main(int argc, char* argv[]) {
  Apparent_All();
  return 0;
}
