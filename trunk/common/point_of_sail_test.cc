// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
#include "common/point_of_sail.h"

#include <math.h>
#include <stdio.h>
#include "common/apparent.h"
#include "common/convert.h"
#include "common/delta_angle.h"
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

void SetEnvWithoutReset(double alpha_wind_true_deg, double mag_wind_true,
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
}

void SetEnv(double alpha_wind_true_deg, double mag_wind_true,
            double phi_z_boat_deg, double mag_boat,
            EnvT* env) {
  SetEnvWithoutReset(alpha_wind_true_deg, mag_wind_true,
         phi_z_boat_deg, mag_boat,
         env);
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
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(Deg2Rad(JibeZoneWidth), sailable);

  // Test that the sector cases have no gaps.
  // 4 test with alpha star exacly at the limit boundaries.
  double alpha_star = 2.268928027592628460240576;
  SetEnv(0, 10, alpha_star, 2, &env);
  sailable = p.SailableHeading(alpha_star,
                               env.alpha_true,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star, sailable);

  alpha_star = -2.268928027592628460240576;
  SetEnv(0, 10, alpha_star, 2, &env);
  sailable = p.SailableHeading(alpha_star,
                               env.alpha_true,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star, sailable);
  alpha_star = -0.349065850398865951120797;
  SetEnv(0, 10, alpha_star, 2, &env);
  sailable = p.SailableHeading(alpha_star,
                               env.alpha_true,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star, sailable);
  alpha_star = 0.349065850398865951120796;
  SetEnv(0, 10, alpha_star, 2, &env);
  sailable = p.SailableHeading(alpha_star,
                               env.alpha_true,
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
  EXPECT_FLOAT_EQ(0, Test(&sailable, -12, env, -JibeZoneWidth));
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
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 1));
  printf("HH");
  SetEnv(-179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 1));  // still sticky
  printf("HH");
  SetEnv(-175, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 5));  // flipped, 5 degree hysteresis
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
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 30));

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
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 6));  // flipped
  SetEnv(-165, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 15));
  SetEnv(-160, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 20));
  SetEnv(-155, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 25));
  SetEnv(-150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 30));
  SetEnv(150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 30));

  boat_speed = 2.5;
  env.delta_app = Deg2Rad(0);             // Stable wind
  SetEnv(120, 10, 0, boat_speed, &env);   // Still go North, Reaching.
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, 0));
  SetEnv(180, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg()));
  SetEnv(179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 1));
  SetEnv(-179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 1));  // still sticky
  SetEnv(-175, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 5));  // still sticky
  SetEnv(-170, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 10));  // flipped
  SetEnv(-165, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 15));
  SetEnv(-160, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 20));
  SetEnv(-155, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 25));
  SetEnv(-150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 30));
  SetEnv(150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 30));

  // Tests of the buffers for apparent wind peaks
  boat_speed = 0;
  SetEnv(179, 10, 0, boat_speed, &env);
  env.delta_app = Deg2Rad(-10);  // So the wind turned left by 10 degrees

  SetEnvWithoutReset(179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 1));
  SetEnvWithoutReset(-179, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 1));  // still sticky
  SetEnvWithoutReset(-175, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() + 5));  // still sticky
  SetEnvWithoutReset(-174, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 6));  // flipped
  SetEnvWithoutReset(-173, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 7));  // flipped
  SetEnvWithoutReset(-172, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 8));  // flipped
  SetEnvWithoutReset(-171, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 9));  // 5 degree apparent tack zone reduction
  SetEnvWithoutReset(-165, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 15));
  env.delta_app = Deg2Rad(0);  // Test of slow decay
  SetEnvWithoutReset(-160, 10, 0, boat_speed, &env);
  // The change rate depends on the decay definition in SailableHeading().
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 20));
  SetEnv(-155, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 25));
  SetEnvWithoutReset(-150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, -TackZoneDeg() + 30));
  SetEnvWithoutReset(150, 10, 0, boat_speed, &env);
  EXPECT_FLOAT_EQ(0, Test(&sailable, 0, env, TackZoneDeg() - 30));


  boat_speed = 2.5;
  for (double delta_app = -180; delta_app < 181; delta_app += 0.1673567) {
    env.delta_app = Deg2Rad(delta_app);
    SetEnv(0, 10, 0, boat_speed, &env);
    Test(&sailable, 0, env, 0);
    EXPECT_TRUE(Rad2Deg(fabs(DeltaOldNewRad(sailable, 0))) < 60);
    printf ("BestWay: %lf %lf\n", delta_app, Rad2Deg(sailable));
  }

  for (double delta_app = 180; delta_app > -181; delta_app -= 0.1673567) {
    env.delta_app = Deg2Rad(delta_app);
    SetEnv(0, 10, 0, boat_speed, &env);
    Test(&sailable, 0, env, 0);
    EXPECT_TRUE(Rad2Deg(fabs(DeltaOldNewRad(sailable, 0))) < 60);
    printf ("BestWay: %lf %lf\n", delta_app, Rad2Deg(sailable));
  }

  alpha_star = 2.268928027592628460240576; // at the limit 1
  // mag_app = 0.001 switches off the processing of the apparent wind and
  // the logic that corrects the desired bearing.
  SetEnv(0, 10, alpha_star, 2, &env);
  sailable = p.SailableHeading(alpha_star,
                               env.alpha_true,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star, sailable);

  // Do not sail into the forbidden tack zone
  sailable = p.SailableHeading(alpha_star + 0.2,
                               env.alpha_true,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star, sailable);

  // But go off the limit left.
  sailable = p.SailableHeading(alpha_star - 0.2,
                               env.alpha_true,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star - 0.2, sailable);

  // The same on the other side.
  alpha_star = -2.268928027592628460240576; // at the limit 2
  // mag_app = 0.001 switches off the processing of the apparent wind and
  // the logic that corrects the desired bearing.
  SetEnv(0, 10, alpha_star, 2, &env);
  sailable = p.SailableHeading(alpha_star,
                               env.alpha_true,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star, sailable);

  // Do not sail into the forbidden tack zone
  sailable = p.SailableHeading(alpha_star - 0.2,
                               env.alpha_true,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star, sailable);

  // But go off the limit left.
  sailable = p.SailableHeading(alpha_star + 0.2,
                               env.alpha_true,
                               sailable,
                               &sector,
                               &target);
  EXPECT_FLOAT_EQ(alpha_star + 0.2, sailable);
}

TEST(PointOfSailTest, AntiWindGust) {
  p.Reset();
  // No effect when running or when the wind speed is too small.
  Angle alpha_app = deg(90);
  double mag_app = 1;
  EXPECT_FLOAT_EQ(0, p.AntiWindGust(JibeStar, alpha_app, mag_app).deg());
  EXPECT_FLOAT_EQ(0, p.AntiWindGust(ReachStar, alpha_app, 0).deg());
  // No effect as long as the wind is not too frontal.
  alpha_app = deg(90);
  EXPECT_FLOAT_EQ(0, p.AntiWindGust(ReachStar, alpha_app, mag_app).deg());
  alpha_app = deg(135);
  EXPECT_FLOAT_EQ(0, p.AntiWindGust(ReachStar, alpha_app, mag_app).deg());
  // Now fall off right:
  alpha_app = deg(140);
  EXPECT_FLOAT_EQ(0, p.AntiWindGust(ReachStar, alpha_app, mag_app).deg());
  p.Reset();
  alpha_app = deg(150);
  EXPECT_FLOAT_EQ(8, p.AntiWindGust(ReachStar, alpha_app, mag_app).deg());
  p.Reset();
  alpha_app = deg(179);
  EXPECT_FLOAT_EQ(37, p.AntiWindGust(ReachStar, alpha_app, mag_app).deg());
  // memory in the buffer decays
  alpha_app = deg(90);
  EXPECT_GT(37, p.AntiWindGust(ReachStar, alpha_app, mag_app).deg());
  alpha_app = deg(220);
  EXPECT_FLOAT_EQ(45, p.AntiWindGust(ReachStar, alpha_app, mag_app).deg());
  // Result would be 46 deg, but is clipped at 45 deg.

  // Fall off left.
  alpha_app = deg(-90);
  EXPECT_FLOAT_EQ(0, p.AntiWindGust(ReachPort, alpha_app, mag_app).deg());
  p.Reset();
  alpha_app = deg(-135);
  EXPECT_FLOAT_EQ(0, p.AntiWindGust(ReachPort, alpha_app, mag_app).deg());
  p.Reset();
  alpha_app = deg(-140);
  EXPECT_FLOAT_EQ(0, p.AntiWindGust(ReachPort, alpha_app, mag_app).deg());
  p.Reset();
  alpha_app = deg(-150);
  EXPECT_FLOAT_EQ(-8, p.AntiWindGust(ReachPort, alpha_app, mag_app).deg());
  p.Reset();
  alpha_app = deg(-179);
  EXPECT_FLOAT_EQ(-37, p.AntiWindGust(ReachPort, alpha_app, mag_app).deg());
  // memory in the buffer decays
  alpha_app = deg(-90);
  EXPECT_LT(-37, p.AntiWindGust(ReachPort, alpha_app, mag_app).deg());
  p.Reset();
  alpha_app = deg(171);
  EXPECT_FLOAT_EQ(-45, p.AntiWindGust(ReachPort, alpha_app, mag_app).deg());
  // clip at -45 deg
}


TEST(PointOfSailTest, OffsetFilter) {
  Angle memory;
  const Angle decay = deg(1 * 0.1);
  Angle out = deg(-1);
  Angle in = deg(1.03);  // recovery within 1 second (10 ticks).
  for (int n = 0; n < 3; ++n) {
    out = FilterOffset(in, decay, &memory);  // recovery within 1 second (10 ticks).
    printf("%lf -> %lf\n" , in.deg(), out.deg());
    EXPECT_ANGLE_EQ(in, out);
  }
  in = 0;
  for (int n = 0; n < 20; ++n) {
    out = FilterOffset(in, decay, &memory);  // recovery within 1 second (10 ticks).
    printf("%lf -> %lf\n" , in.deg(), out.deg());
    if (10 == n)
      EXPECT_EQ(0, out);
  }
  in = deg(-1);
  for (int n = 0; n < 3; ++n) {
    out = FilterOffset(in, decay, &memory);  // recovery within 1 second (10 ticks).
    printf("%lf -> %lf\n" , in.deg(), out.deg());
    EXPECT_ANGLE_EQ(in, out);
  }
  in = 0;
  for (int n = 0; n < 20; ++n) {
    out = FilterOffset(in, decay, &memory);  // recovery within 1 second (10 ticks).
    printf("%lf -> %lf\n" , in.deg(), out.deg());
    if (10 == n)
      EXPECT_EQ(0, out);
  }
  in = deg(-1);
  for (int n = 0; n < 3; ++n) {
    out = FilterOffset(in, decay, &memory);  // recovery within 1 second (10 ticks).
    printf("%lf -> %lf\n" , in.deg(), out.deg());
    EXPECT_ANGLE_EQ(in, out);
  }
  in = deg(1);
  for (int n = 0; n < 20; ++n) {
    out = FilterOffset(in, decay, &memory);  // recovery within 1 second (10 ticks).
    printf("%lf -> %lf\n" , in.deg(), out.deg());
    EXPECT_ANGLE_EQ(in, out);
  }

}


int main(int argc, char* argv[]) {
  PointOfSailTest_AntiWindGust();
  PointOfSailTest_SailableHeading();
  PointOfSailTest_OffsetFilter();
  return 0;
}
