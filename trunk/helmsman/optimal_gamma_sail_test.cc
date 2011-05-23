// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/optimal_gamma_sail.h"

#include "common/testing.h"
#include "common/convert.h"

// Returns optimal gamma sail for the given
// direction of the apparent wind vector relative to the boats x-axis
// (in degrees, 90 degrees is wind from port side, 0 degree is wind
// from behind) in [0, 360) and
// the wind speed in m/s. wind_speed >= 0 
double OptimalGammaSail(double alpha_apparent_wind, double wind_speed);

// As OptimalGammaSail. Additionally the forward force F_x in Newton and
// the heel angle, rotation around the x-axis, indegrees is produced. 
void OptimalTriplett(double alpha_apparent_wind,
                     double magnitude_wind,
                     double* gamma_sail,
                     double* force,
                     double* heel);

TEST(OptimalGammaSail, All) {

double wind_speed = 5;
double gamma_sail = 0;
double force = 0;
double heel = 0;



printf("apparent/degree gamma_sail/degree, force/N, heel/degree\n");
for (double apparent = -20; apparent < 400; apparent += 0.742) {
  OptimalTriplett(Deg2Rad(apparent),
                  wind_speed,
                  &gamma_sail,
                  &force,
                  &heel);
  printf("%15g %15g %15g %15g\n",
         apparent, Rad2Deg(gamma_sail), force, Rad2Deg(heel));                
  EXPECT_GT(force, -100);
}

printf("apparent/degree gamma_sail/degree, force/N, heel/degree\n");
for (double apparent = 1; apparent < 180; apparent += 0.5) {
  OptimalTriplett(Deg2Rad(apparent),
                  wind_speed,
                  &gamma_sail,
                  &force,
                  &heel);
  printf("%15g %15g %15g %15g\n",
         apparent, Rad2Deg(gamma_sail), force, Rad2Deg(heel));                
  EXPECT_LT(gamma_sail, -15);
  EXPECT_GT(gamma_sail, -120);
  EXPECT_GT(force, 0);
  EXPECT_GT(heel, -2);
}

printf("apparent/degree gamma_sail/degree, force/N, heel/degree\n");
for (double apparent = -1; apparent > -180; apparent -= 0.5) {
  OptimalTriplett(Deg2Rad(apparent),
                  wind_speed,
                  &gamma_sail,
                  &force,
                  &heel);
  printf("%15g %15g %15g %15g\n",
         apparent, Rad2Deg(gamma_sail), force, Rad2Deg(heel));                
  EXPECT_GT(gamma_sail, 15);
  EXPECT_LT(gamma_sail, 120);
  EXPECT_GT(force, 0);
  EXPECT_LT(heel, 2);
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
EXPECT_LT(3.9 * force, force2);
EXPECT_GT(4.1 * force, force2);

}

int main(int argc, char* argv[]) {
  OptimalGammaSail_All();
  return 0;
}
