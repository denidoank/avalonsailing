// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011


#include "apparent.h"

#include "lib/convert.h"
#include "lib/normalize.h"

#include "lib/testing/testing.h"


TEST(Apparent, All) {
  Polar true_wind(0, 10);
  Polar motion(0.1, -2);
  double angle;
  double mag;
  Apparent(true_wind.AngleRad(), true_wind.Mag(),
           motion.AngleRad(), motion.Mag(),
           motion.AngleRad(),
           &angle, &mag);
  EXPECT_FLOAT_EQ(-0.0833488, angle);
  EXPECT_FLOAT_EQ(11.9917, mag);  
 
  Polar app(0, 0);
  // Use motion.AngleRad() here for phi_z
  ApparentPolar(true_wind, motion, motion.AngleRad(), &app);
  EXPECT_FLOAT_EQ(-0.0833488, app.AngleRad());
  EXPECT_FLOAT_EQ(11.9917, app.Mag());

  true_wind = Polar(Deg2Rad(170), 10);
  motion = Polar(Deg2Rad(-100.421), -2.652);
  // Use motion.AngleRad() here for phi_z
  ApparentPolar(true_wind, motion, motion.AngleRad(), &app);
  EXPECT_FLOAT_EQ(Deg2Rad(-74.754), app.AngleRad());
  EXPECT_FLOAT_EQ(10.3645, app.Mag());
  
  Polar true_wind_result(0, 0);
  // Use motion.AngleRad() here for phi_z
  TruePolar(app, motion, motion.AngleRad(), &true_wind_result);
  EXPECT_FLOAT_EQ(Deg2Rad(170), true_wind_result.AngleRad());
  EXPECT_FLOAT_EQ(10.0, true_wind_result.Mag());
  
  motion = Polar(Deg2Rad(-100.421), -2.652);
  for (double true_wind_angle = -400;
       true_wind_angle < 400;
       true_wind_angle += 0.01252) {
    true_wind = Polar(Deg2Rad(true_wind_angle), 10);
    ApparentPolar(true_wind, motion, motion.AngleRad(), &app);
    
    Polar true_wind_result(0, 0);
    // Use motion.AngleRad() here for phi_z
    TruePolar(app, motion, motion.AngleRad(), &true_wind_result);
    EXPECT_FLOAT_EQ(SymmetricRad(Deg2Rad(true_wind_angle)),
                    true_wind_result.AngleRad());
    EXPECT_FLOAT_EQ(10.0, true_wind_result.Mag());     
  }
  
  // With phi_z
  motion = Polar(Deg2Rad(-179.861), -4.652);
  for (double phi_z = Deg2Rad(-10); phi_z < Deg2Rad(10); phi_z += 0.123) {
    for (double true_wind_angle = -400;
         true_wind_angle < 400;
         true_wind_angle += 0.1752) {
      true_wind = Polar(Deg2Rad(true_wind_angle), 10);
      ApparentPolar(true_wind, motion, phi_z, &app);
      
      Polar true_wind_result(0, 0);
      TruePolar(app, motion, phi_z, &true_wind_result);
      EXPECT_FLOAT_EQ(SymmetricRad(Deg2Rad(true_wind_angle)),
                      true_wind_result.AngleRad());
      EXPECT_FLOAT_EQ(10.0, true_wind_result.Mag());     
    }
  }
}

int main(int argc, char* argv[]) {
  Apparent_All();
  return 0;
}
