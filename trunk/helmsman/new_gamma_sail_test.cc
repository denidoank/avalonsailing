// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/new_gamma_sail.h"

#include "common/angle.h"
#include "common/apparent.h"
#include "common/convert.h"
#include "common/normalize.h"
#include "common/polar_diagram.h"
#include "common/sign.h"
#include "lib/testing/testing.h"
#include "helmsman/maneuver_type.h"

Polar app(double alpha_deg, double mag) {
  Polar apparent_wind(deg(alpha_deg), mag);
  return apparent_wind;
}

ATEST(NewGammaSail, NewTack) {
  double aoa_optimal = Deg2Rad(10);
  SailController sail_controller;
  sail_controller.SetOptimalAngleOfAttack(aoa_optimal);

  double alpha_true = M_PI;  // North wind vector
  double mag_true = 10;
  double alpha_boat = TackZoneRad();
  double mag_boat = 0.5;
  double angle_app;
  double mag_app;
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  sail_controller.SetAppSign(SignNotZero(angle_app));
  double old_gamma_sail =
      sail_controller.StableGammaSailFromApparent(app(angle_app, mag_app)).rad();

  double new_gamma_sail = -1;
  double delta_gamma_sail = -1;
  NewGammaSail(old_gamma_sail,  // -37.87 degree
               kTack,
               0,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_FLOAT_EQ(-old_gamma_sail, new_gamma_sail);
  EXPECT_FLOAT_EQ(-2*old_gamma_sail, delta_gamma_sail);

  NewGammaSail(-old_gamma_sail,
               kTack,
               0,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_FLOAT_EQ(old_gamma_sail, new_gamma_sail);
  EXPECT_FLOAT_EQ(2*old_gamma_sail, delta_gamma_sail);

  // different wind speed.
  mag_true = 3;
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  old_gamma_sail =
      sail_controller.StableGammaSailFromApparent(app(angle_app, mag_app)).rad();

  NewGammaSail(old_gamma_sail,  // -33.4 degree
               kTack,
               0,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_FLOAT_EQ(-old_gamma_sail, new_gamma_sail);
  EXPECT_FLOAT_EQ(-2*old_gamma_sail, delta_gamma_sail);

  NewGammaSail(-old_gamma_sail,
               kTack,
               0,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_FLOAT_EQ(old_gamma_sail, new_gamma_sail);
  EXPECT_FLOAT_EQ(2*old_gamma_sail, delta_gamma_sail);

  NewGammaSail(-old_gamma_sail,
               kTack,
               0.1,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_FLOAT_EQ(old_gamma_sail - 0.1, new_gamma_sail);
  EXPECT_FLOAT_EQ(2*old_gamma_sail - 0.1, delta_gamma_sail);

  // overshoot
  NewGammaSail(old_gamma_sail,
               kTack,
               0.1,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_FLOAT_EQ(-old_gamma_sail + 0.1, new_gamma_sail);
  EXPECT_FLOAT_EQ(-2 * old_gamma_sail + 0.1, delta_gamma_sail);
}

ATEST(NewGammaSail, NewJibe) {
  printf("Jibe\n");
  double aoa_optimal = Deg2Rad(10);
  SailController sail_controller;
  sail_controller.SetOptimalAngleOfAttack(aoa_optimal);

  double alpha_true = Deg2Rad(180);  // North wind vector
  double mag_true = 10;
  double alpha_boat = JibeZoneRad();
  double mag_boat = 0.5;
  double angle_app;
  double mag_app;
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  double old_gamma_sail =
      sail_controller.StableGammaSailFromApparent(app(angle_app, mag_app)).rad();
  printf("old_gamma_sail %lf deg\n", Rad2Deg(old_gamma_sail));
  double new_gamma_sail = -1;
  double delta_gamma_sail = -1;
  NewGammaSail(old_gamma_sail,  // -85.1 degree
               kJibe,
               0,
               &new_gamma_sail,
               &delta_gamma_sail);
  printf("new_gamma_sail %lf deg\n", Rad2Deg(new_gamma_sail));
  printf("delta_gamma_sail %lf deg\n", Rad2Deg(delta_gamma_sail));
  EXPECT_FLOAT_EQ(-old_gamma_sail, new_gamma_sail);
  EXPECT_FLOAT_EQ(-2 * old_gamma_sail - 2 * M_PI, delta_gamma_sail);

  NewGammaSail(-old_gamma_sail,   // 85.1 degree
               kJibe,
               0,
               &new_gamma_sail,
               &delta_gamma_sail);
  printf("delta_gamma_sail %lf deg\n", Rad2Deg(delta_gamma_sail));
  EXPECT_FLOAT_EQ(old_gamma_sail, new_gamma_sail);
  EXPECT_FLOAT_EQ(2 * old_gamma_sail + 2 * M_PI, delta_gamma_sail);

  // different wind speed.
  mag_true = 3;
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  old_gamma_sail =
      sail_controller.StableGammaSailFromApparent(app(angle_app, mag_app)).rad();

  NewGammaSail(old_gamma_sail,  // -84 degree
               kJibe,
               0,
               &new_gamma_sail,
               &delta_gamma_sail);
  printf("delta_gamma_sail %lf deg\n", Rad2Deg(delta_gamma_sail));
  EXPECT_FLOAT_EQ(-old_gamma_sail, new_gamma_sail);
  EXPECT_FLOAT_EQ(-2 * old_gamma_sail - 2 * M_PI, delta_gamma_sail);

  NewGammaSail(-old_gamma_sail,  // 84 degree
               kJibe,
               0,
               &new_gamma_sail,
               &delta_gamma_sail);
  printf("delta_gamma_sail %lf deg\n", Rad2Deg(delta_gamma_sail));
  EXPECT_FLOAT_EQ(old_gamma_sail, new_gamma_sail);
  EXPECT_FLOAT_EQ(2 * old_gamma_sail + 2 * M_PI, delta_gamma_sail);
  // No overshoot for jibe
  NewGammaSail(-old_gamma_sail,  // 84 degree
               kJibe,
               0.1,
               &new_gamma_sail,
               &delta_gamma_sail);
  printf("delta_gamma_sail %lf deg\n", Rad2Deg(delta_gamma_sail));
  EXPECT_FLOAT_EQ(old_gamma_sail, new_gamma_sail);
  EXPECT_FLOAT_EQ(2 * old_gamma_sail + 2 * M_PI, delta_gamma_sail);
}

int main() {
  return testing::RunAllTests();
  return 0;
}
