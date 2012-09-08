// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
#include "common/point_of_sail.h"

#include <math.h>
#include <stdio.h>
#include "common/apparent.h"
#include "common/convert.h"
#include "common/polar.h"
#include "common/polar_diagram.h"

#include "lib/testing/testing.h"

int debug = 1;
PointOfSail p;

typedef struct Env {
  double alpha_true;
  double alpha_app;
  double mag_app;
  double phi_z;
  double delta_app;  // difference of apparent wind in relation to true wind
} EnvT;

namespace {
double Test(double* sailable, double star, const EnvT& env, double expected) {
  printf("Want to sail %lf at %lf true wind.\n", star, Rad2Deg(env.alpha_true));
  SectorT sector;
  double target;
  *sailable = p.SailableHeading(Deg2Rad(star),
                                env.alpha_true,
                                env.alpha_app + env.delta_app,
                                env.mag_app,
                                env.phi_z,
                                *sailable,
                                &sector,
                                &target);
  EXPECT_IN_INTERVAL(-M_PI, target, M_PI);
  EXPECT_IN_INTERVAL(-M_PI, *sailable, M_PI);
  if (fabs(*sailable - Deg2Rad(expected)) > 1E-4) {
    printf("sailable: %lf deg\n", Rad2Deg(*sailable));
    printf("expected: %lf deg\n", expected);
  }
  return *sailable - Deg2Rad(expected);
}

void SetEnv(double alpha_wind_true_deg, double mag_wind_true,
            double phi_z_boat_deg, double mag_boat,
            EnvT* env) {
  Polar wind_true(Deg2Rad(alpha_wind_true_deg), mag_wind_true);
  Polar boat(Deg2Rad(phi_z_boat_deg), mag_boat);

  env->alpha_true = Deg2Rad(alpha_wind_true_deg);
  env->phi_z = Deg2Rad(phi_z_boat_deg);

  Apparent(Deg2Rad(alpha_wind_true_deg), mag_wind_true,
           Deg2Rad(phi_z_boat_deg), mag_boat,
           phi_z_boat_deg,
           &env->alpha_app, &env->mag_app);

  printf("Env: T:%lf A:%lf delta:%lf |A:%lf| phi_z:%lf\n",
         env->alpha_true, env->alpha_app, env->delta_app,
         env->mag_app, env->phi_z);
  p.Reset();
}

}  // namespace

TEST(PointOfSailTest, SailableHeading) {
  double sailable = 0;
  Env env = {0, 0, 10, 0, 0};  // running

  SetEnv(0, 10, 0, 2, &env);  // Go North, running.

  const double JibeZoneWidth = 180 - JibeZoneDeg();
  SectorT sector;
  double target;
  sailable = p.SailableHeading(Deg2Rad(1),
                                env.alpha_true,
                                env.alpha_app,
                                env.mag_app,
                                env.phi_z,
                                sailable,
                                &sector,
                                &target);
  EXPECT_FLOAT_EQ(Deg2Rad(JibeZoneWidth), sailable);

  // Test that the sector cases have no gaps.
  // 4 test with alpha star exacly at the limit boundaries.
  double alpha_star = 2.268928027592628460240576;
  sailable = p.SailableHeading(alpha_star,
                               env.alpha_true,
                               env.alpha_app,
                               env.mag_app,
                               env.phi_z,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star, sailable);

  alpha_star = -2.268928027592628460240576;
  sailable = p.SailableHeading(alpha_star,
                               env.alpha_true,
                               env.alpha_app,
                               env.mag_app,
                               env.phi_z,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star, sailable);
  alpha_star = -0.349065850398865951120797;
  sailable = p.SailableHeading(alpha_star,
                               env.alpha_true,
                               env.alpha_app,
                               env.mag_app,
                               env.phi_z,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star, sailable);
  alpha_star = 0.349065850398865951120796;
  sailable = p.SailableHeading(alpha_star,
                               env.alpha_true,
                               env.alpha_app,
                               env.mag_app,
                               env.phi_z,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star, sailable);

  //              a*         expected
  EXPECT_FLOAT_EQ(0, Test(&sailable, 45, env,   45));
  EXPECT_FLOAT_EQ(0, Test(&sailable,  0, env,   JibeZoneWidth));
  // Hysteresis effect (stick to the previous decision if the difference is not too big)
  EXPECT_FLOAT_EQ(0, Test(&sailable,  -1, env,  JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable,  -5, env,  JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable,  -8, env,  JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable,  -9, env, -JibeZoneWidth)); // flipped over
  EXPECT_FLOAT_EQ(0, Test(&sailable, -10, env, -JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable, -JibeZoneWidth, env, -JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable, -30, env, -30));
  EXPECT_FLOAT_EQ(0, Test(&sailable,  -9, env, -JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable,  -5, env, -JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable,  -1, env, -JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable,   0, env, -JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable,   1, env, -JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable,   8, env, -JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable,   9, env,  JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable,  15, env,  JibeZoneWidth));
  EXPECT_FLOAT_EQ(0, Test(&sailable,  33, env,  33));

  EXPECT_FLOAT_EQ(0, Test(&sailable,  50, env,  50));
  EXPECT_FLOAT_EQ(0, Test(&sailable,  90, env,  90));
  EXPECT_FLOAT_EQ(0, Test(&sailable,  91, env,  91));
  EXPECT_FLOAT_EQ(0, Test(&sailable, -50, env, -50));
  EXPECT_FLOAT_EQ(0, Test(&sailable, -90, env, -90));
  EXPECT_FLOAT_EQ(0, Test(&sailable, -91, env, -91));

  SetEnv(90, 10, 0, 2, &env);   // Still go North, Reaching.
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, 0));
  SetEnv(120, 10, 0, 2, &env);  // Still go North, Reaching.
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, 0));

  SetEnv(120, 10, 0, 2, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, 0));
  SetEnv(180, 10, 0, 2, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0.00001, env, TackZoneDeg()));
  // for boat speed 0 the apparent wind direction is equal to the true wind direction.
  double boat_speed = 0;
  SetEnv(179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 1));
  SetEnv(-179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 1));  // still sticky
  SetEnv(-175, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 5));  // still sticky, 5 degree hysteresis
  SetEnv(-174, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 6));  // flipped
  SetEnv(-165, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 15));
  SetEnv(-160, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 20));
  SetEnv(-155, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 25));
  SetEnv(-150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 30));
  SetEnv(90, 10, 0, boat_speed, &env);
  printf("AA");
  EXPECT_FLOAT_EQ(0, Test(&sailable, 60, env, 60));

  // So the wind turned right by 3 degrees
  env.delta_app = Deg2Rad(3);
  // But the TackZone is reduced for the apparent wind by more than 3 degrees.
  SetEnv(179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 1 + 3));
  printf("HH");
  SetEnv(-179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 1 + 3));  // still sticky
  printf("HH");
  SetEnv(-175, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 5));  // flipped, 5 degree hysteresis
  printf("HH");
  SetEnv(-174, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 6));  // flipped
  SetEnv(-165, 10, 0, boat_speed, &env);
  printf("HH");
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 15));
  SetEnv(-160, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 20));
  printf("HH");
  SetEnv(-155, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 25));
  SetEnv(-150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 30));
  SetEnv(150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 30 + 3));

  printf("The other way round\n\n");

  SetEnv(179, 10, 0, boat_speed, &env);
  env.delta_app = Deg2Rad(-3);  // So the wind turned left by 3 degrees

  SetEnv(179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 1));
  SetEnv(-179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 1));  // still sticky
  SetEnv(-175, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 5));  // still sticky
  SetEnv(-174, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 6));  // flipped
  SetEnv(-165, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 15 - 3));
  SetEnv(-160, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 20 - 3));
  SetEnv(-155, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 25 - 3));
  SetEnv(-150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 30 - 3));
  SetEnv(150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 30));

  boat_speed = 2.5;
  env.delta_app = Deg2Rad(0);             // Stable wind
  SetEnv(120, 10, 0, boat_speed, &env);   // Still go North, Reaching.
  EXPECT_FLOAT_EQ(0.0155927, Test(&sailable, 0, env, 0));
  SetEnv(180, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg()));
  SetEnv(179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0.00349057, Test(&sailable, 0, env, TackZoneDeg() - 1));
  SetEnv(-179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 1));  // still sticky
  SetEnv(-175, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 5));  // still sticky
  SetEnv(-170, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(-0.0348214, Test(&sailable, 0, env, -TackZoneDeg() + 10));  // flipped
  SetEnv(-165, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(-0.0520719, Test(&sailable, 0, env, -TackZoneDeg() + 15));
  SetEnv(-160, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(-0.0691288, Test(&sailable, 0, env, -TackZoneDeg() + 20));
  SetEnv(-155, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(-0.0859256, Test(&sailable, 0, env, -TackZoneDeg() + 25));
  SetEnv(-150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(-0.102394, Test(&sailable, 0, env, -TackZoneDeg() + 30));
  SetEnv(150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0.102394, Test(&sailable, 0, env, TackZoneDeg() - 30));
}


int main(int argc, char* argv[]) {
  PointOfSailTest_SailableHeading();
  return 0;
}
