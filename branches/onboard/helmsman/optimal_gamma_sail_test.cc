// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "optimal_gamma_sail.h"

#include "lib/convert.h"
#include "lib/testing/testing.h"

TEST(OptimalGammaSail, All) {
  double wind_speed = 5;
  double gamma_sail = 0;
  double force = 0;
  double heel = 0;
  double gamma_sail_2 = 0;

  printf("     apparent/degree gamma_sail/degree, force/N,       heel/degree\n");
  for (double apparent = -20; apparent < 400; apparent += 0.742) {
    OptimalTriplett(Deg2Rad(apparent),
                    wind_speed,
                    &gamma_sail,
                    &force,
                    &heel);
    gamma_sail_2 = OptimalGammaSail(Deg2Rad(apparent), wind_speed);
    EXPECT_FLOAT_EQ(gamma_sail, gamma_sail_2);              
    printf("%15g %15g %15g %15g\n",
           apparent, Rad2Deg(gamma_sail), force, Rad2Deg(heel));                
    EXPECT_GT(force, -100);
  }

  printf("     apparent/degree gamma_sail/degree, force/N,       heel/degree\n");
  for (double apparent = 0; apparent < 140; apparent += 0.5) {
    OptimalTriplett(Deg2Rad(apparent),
                    wind_speed,
                    &gamma_sail,
                    &force,
                    &heel);
    gamma_sail_2 = OptimalGammaSail(Deg2Rad(apparent), wind_speed);
    EXPECT_FLOAT_EQ(gamma_sail, gamma_sail_2);              
    printf("%15g %15g %15g %15g\n",
           apparent, Rad2Deg(gamma_sail), force, Rad2Deg(heel));                
    EXPECT_LT(gamma_sail, Deg2Rad(-15));
    EXPECT_GT(gamma_sail, Deg2Rad(-120));
    EXPECT_GT(force, 0);
    EXPECT_GT(heel, Deg2Rad(-2));
  }

  printf("     apparent/degree gamma_sail/degree, force/N,       heel/degree\n");
  for (double apparent = -0.5; apparent > -140; apparent -= 0.5) {
    OptimalTriplett(Deg2Rad(apparent),
                    wind_speed,
                    &gamma_sail,
                    &force,
                    &heel);
    gamma_sail_2 = OptimalGammaSail(Deg2Rad(apparent), wind_speed);
    EXPECT_FLOAT_EQ(gamma_sail, gamma_sail_2);              
    printf("%15g %15g %15g %15g\n",
           apparent, Rad2Deg(gamma_sail), force, Rad2Deg(heel));                
    EXPECT_GT(gamma_sail, Deg2Rad(15));
    EXPECT_LT(gamma_sail, Deg2Rad(120));
    EXPECT_GT(force, 0);
    EXPECT_LT(heel, Deg2Rad(2));
  }

  double apparent = 0;
  wind_speed = 5;
  OptimalTriplett(apparent,
                  wind_speed,
                  &gamma_sail,
                  &force,
                  &heel);
  // double wind speed, four times the force
  wind_speed *= 2;
  double force2;
  OptimalTriplett(apparent,
                  wind_speed,
                  &gamma_sail,
                  &force2,
                  &heel);
  EXPECT_LT(3.6 * force, force2);
  EXPECT_GT(4.4 * force, force2);
}

int main(int argc, char* argv[]) {
  OptimalGammaSail_All();
  return 0;
}
