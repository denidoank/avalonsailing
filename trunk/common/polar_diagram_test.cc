// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
#include "common/polar_diagram.h"

#include <math.h>
#include <stdio.h>
#include "common/apparent.h"
#include "common/convert.h"
#include "common/polar.h"

#include "lib/testing/testing.h"

int debug = 1;

// print polar diagram as x-y-chart.
// Columns: angle/degree, speed_magnitude/m/s, speed_x/m/s, speed_y/m/s,
// tack_zone, jibe_zone,
// and xy values for constant speed circles at the 50%, 75% and 100% of the
// wind speed.
void LogPolarDiagram() {
  bool dead_zone_tack;
  bool dead_zone_jibe;
  double boat_speed_m_s;
  for (double wind_speed_m_s = 5; wind_speed_m_s <= 10; wind_speed_m_s += 5) {
    printf("\n For wind_speed_m_s: %lg m/s\n", wind_speed_m_s);
    printf("angle/degree,speed_magnitude/ m/s,speed_x/k m/s,speed_y/ m/s,"
           "tack_zone,jibe_zone,"
           "wind_speed_50_x/m/s,wind_speed_50_y/m/s,"
           "wind_speed_75_x/m/s,wind_speed_75_y/m/s,"
           "wind_speed_100_x/m/s,wind_speed_100_y/m/s\n");
    for (double alpha = -180.0; alpha <= 180; alpha += 2) {
      double rad = Deg2Rad(alpha);
      ReadPolarDiagram(alpha,
                       wind_speed_m_s,
                       &dead_zone_tack,
                       &dead_zone_jibe,
                       &boat_speed_m_s);
      printf("%lg,%lg,%lg,%lg,%d,%d,%lg,%lg,%lg,%lg,%lg,%lg\n",
             alpha, boat_speed_m_s, boat_speed_m_s * sin(rad), boat_speed_m_s * cos(rad),
             dead_zone_tack ? 1 : 0, dead_zone_jibe ? 1 : 0,
             0.5 * wind_speed_m_s * sin(rad), 0.5 * wind_speed_m_s * cos(rad),
             0.75 * wind_speed_m_s * sin(rad), 0.75 * wind_speed_m_s * cos(rad),
             1.0 * wind_speed_m_s * sin(rad), 1.0 * wind_speed_m_s * cos(rad));
      // printf("%lg,%lg,\n", alpha, boat_speed_m_s);
    }
  }

  printf("\n For angle 90 degrees wind_speed_m_s: 0 - 20 m/s\n true wind speed / m/s, boat speed / m/s\n");
  for (double wind_speed_m_s = 0; wind_speed_m_s <= 30; wind_speed_m_s += 0.25) {
    ReadPolarDiagram(90,
                     wind_speed_m_s,
                     &dead_zone_tack,
                     &dead_zone_jibe,
                     &boat_speed_m_s);
    printf("%lg,%lg\n", wind_speed_m_s, boat_speed_m_s);

  }
}


double TestSpeed(double angle, double wind_speed_m_s) {
  double speed;
  bool f1, f2;
  printf("TestSpeed %lf %lf\n", angle, wind_speed_m_s);
  ReadPolarDiagram(angle,
                   wind_speed_m_s,
                   &f1,
                   &f2,
                   &speed);
  return speed;
}

TEST(PolarDiagramTest, All) {
const double w_m_s = 4.5;


/*
40 0.12 is in Tack zone
50 0.165
72 0.3
110 0.4
150 0.35
165 0.32
175 0.28
180 0.27 in Jibe zone
*/

double v = 0.165 * w_m_s;
EXPECT_IN_INTERVAL(v - 0.1, TestSpeed(50, w_m_s), v + 0.1);
v = 0.3 * w_m_s;
EXPECT_IN_INTERVAL(v - 0.1, TestSpeed(72, w_m_s), v + 0.1);
v = 0.4 * w_m_s;
EXPECT_IN_INTERVAL(v - 0.1, TestSpeed(110, w_m_s), v + 0.1);
v = 0.35 * w_m_s;
EXPECT_IN_INTERVAL(v - 0.1, TestSpeed(150, w_m_s), v + 0.1);
v = 0.32 * w_m_s;
EXPECT_IN_INTERVAL(v - 0.1, TestSpeed(165, w_m_s), v + 0.1);
v = 0.3 * w_m_s;  // in Jibe zone
EXPECT_IN_INTERVAL(v - 0.1, TestSpeed(175, w_m_s), v + 0.1);
v = 0.3 * w_m_s;  // in Jibe zone
EXPECT_IN_INTERVAL(v - 0.5, TestSpeed(180, w_m_s), v + 0.5);

double wind_speed_m_s = 10;
bool dead_zone_tack;
bool dead_zone_jibe;
double close_reach_speed;

double tack_zone_angle = TackZoneDeg();
ReadPolarDiagram(tack_zone_angle,
                 wind_speed_m_s,
                 &dead_zone_tack,
                 &dead_zone_jibe,
                 &close_reach_speed);
EXPECT_EQ(dead_zone_tack, false);
EXPECT_EQ(dead_zone_jibe, false);
EXPECT_IN_INTERVAL(0.1 * wind_speed_m_s, close_reach_speed, 0.12 * wind_speed_m_s);

// speed against the wind
double boat_speed_m_s;
ReadPolarDiagram(0,
                 wind_speed_m_s,
                 &dead_zone_tack,
                 &dead_zone_jibe,
                 &boat_speed_m_s);

EXPECT_EQ(dead_zone_tack, true);
EXPECT_EQ(dead_zone_jibe, false);

EXPECT_FLOAT_EQ(0.748392, boat_speed_m_s);
EXPECT_IN_INTERVAL(0.074 * wind_speed_m_s, boat_speed_m_s, 0.075 * wind_speed_m_s);
EXPECT_IN_INTERVAL(cos(Deg2Rad(tack_zone_angle)) * 0.1 * wind_speed_m_s,
                   boat_speed_m_s,
                   cos(Deg2Rad(tack_zone_angle)) * 0.12 * wind_speed_m_s);


double jibe_zone_angle = JibeZoneDeg();
double jibe_zone_speed;
ReadPolarDiagram(jibe_zone_angle,
                 wind_speed_m_s,
                 &dead_zone_tack,
                 &dead_zone_jibe,
                 &jibe_zone_speed);
EXPECT_EQ(dead_zone_tack, false);
EXPECT_EQ(dead_zone_jibe, false);

// speed with the wind
ReadPolarDiagram(180,
                 wind_speed_m_s,
                 &dead_zone_tack,
                 &dead_zone_jibe,
                 &boat_speed_m_s);

EXPECT_EQ(dead_zone_tack, false);
EXPECT_EQ(dead_zone_jibe, true);

EXPECT_FLOAT_EQ(2.01836, boat_speed_m_s);

EXPECT_FLOAT_EQ(0.391058, TestSpeed(110, 1));
EXPECT_FLOAT_EQ(1.75976,  TestSpeed(110, 4.5));
EXPECT_FLOAT_EQ(2.54949,  TestSpeed(110, 15));
EXPECT_FLOAT_EQ(2.6,      TestSpeed(110, 25));

EXPECT_FLOAT_EQ(JibeZoneDeg(), 180 * JibeZoneRad() / M_PI);
EXPECT_FLOAT_EQ(TackZoneDeg(), 180 * TackZoneRad() / M_PI);

LogPolarDiagram();
}

TEST(PolarDiagramTest, BestSailable) {
  const double JibeZoneWidth = 180 - JibeZoneDeg();
  //                                             alpha_star, alpha_true
  EXPECT_FLOAT_EQ( JibeZoneWidth, BestSailableHeadingDeg(+1, 0));
  EXPECT_FLOAT_EQ(-JibeZoneWidth, BestSailableHeadingDeg(-1, 0));
  EXPECT_FLOAT_EQ(0, BestSailableHeadingDeg(0, 90));
  EXPECT_FLOAT_EQ(0, BestSailableHeadingDeg(0, 120));
  EXPECT_FLOAT_EQ(TackZoneDeg(), BestSailableHeadingDeg(0, 180));
  EXPECT_FLOAT_EQ(TackZoneDeg() - 1, BestSailableHeadingDeg(0, 179));
  EXPECT_FLOAT_EQ(1 - TackZoneDeg(), BestSailableHeadingDeg(0, -179));
  
  EXPECT_FLOAT_EQ(50, BestSailableHeadingDeg(50, 0));
}

namespace {
void Test_0(double* sailable, double star, double alpha_true, double expected) {
  *sailable = BestStableSailableHeading(Deg2Rad(star), Deg2Rad(alpha_true), *sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(expected), *sailable);
}
}  // namespace

TEST(PolarDiagramTest, BestStableSailableHeading) {
  double sailable=0;
  const double JibeZoneWidth = 180 - JibeZoneDeg();
  //                                             alpha_star, alpha_true, previous
  sailable = BestStableSailableHeading(Deg2Rad(1), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ( Deg2Rad(JibeZoneWidth), sailable);

  // Hysteresis effect (stick to the previous decision if the difference is not too big)
  sailable = BestStableSailableHeading(Deg2Rad(-1), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(JibeZoneWidth), sailable);  // would be -JibeZoneWidth
  sailable = BestStableSailableHeading(Deg2Rad(-5), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(JibeZoneWidth), sailable);  // would be -JibeZoneWidth
  sailable = BestStableSailableHeading(Deg2Rad(-10), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(-JibeZoneWidth), sailable);  // flipped over now
  sailable = BestStableSailableHeading(Deg2Rad(-15), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(-JibeZoneWidth), sailable);
  // Out of jibe zone
  sailable = BestStableSailableHeading(Deg2Rad(-25), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(-25), sailable);
  sailable = BestStableSailableHeading(Deg2Rad(-5), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(-JibeZoneWidth), sailable);  // would be -JibeZoneWidth
  sailable = BestStableSailableHeading(Deg2Rad( 5), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(-JibeZoneWidth), sailable);  // would be -JibeZoneWidth
  sailable = BestStableSailableHeading(Deg2Rad( 6), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(-JibeZoneWidth), sailable);  // would be -JibeZoneWidth
  sailable = BestStableSailableHeading(Deg2Rad( 7), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(-JibeZoneWidth), sailable);  // would be -JibeZoneWidth
  sailable = BestStableSailableHeading(Deg2Rad( 8), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(-JibeZoneWidth), sailable);  // would be -JibeZoneWidth
  sailable = BestStableSailableHeading(Deg2Rad( 9), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad( JibeZoneWidth), sailable);  // flipped below 9 degrees
  sailable = BestStableSailableHeading(Deg2Rad(10), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad( JibeZoneWidth), sailable);
  sailable = BestStableSailableHeading(Deg2Rad(45), Deg2Rad(0), sailable);
  EXPECT_FLOAT_EQ(Deg2Rad(45), sailable);

  Test_0(&sailable, 45, 0,   45);
  Test_0(&sailable,  0, 0,   JibeZoneWidth);
  // Hysteresis effect (stick to the previous decision if the difference is not too big)
  Test_0(&sailable,  -1, 0,  JibeZoneWidth);
  Test_0(&sailable,  -5, 0,  JibeZoneWidth);
  Test_0(&sailable,  -8, 0,  JibeZoneWidth);
  Test_0(&sailable,  -9, 0, -JibeZoneWidth); // flipped over
  Test_0(&sailable, -10, 0, -JibeZoneWidth);
  Test_0(&sailable, -JibeZoneWidth, 0, -JibeZoneWidth);
  Test_0(&sailable, -30, 0, -30);
  Test_0(&sailable,  -9, 0, -JibeZoneWidth);
  Test_0(&sailable,  -5, 0, -JibeZoneWidth);
  Test_0(&sailable,  -1, 0, -JibeZoneWidth);
  Test_0(&sailable,   0, 0, -JibeZoneWidth);
  Test_0(&sailable,   1, 0, -JibeZoneWidth);
  Test_0(&sailable,   8, 0, -JibeZoneWidth);
  Test_0(&sailable,   9, 0,  JibeZoneWidth);
  Test_0(&sailable,  15, 0,  JibeZoneWidth);
  Test_0(&sailable,  33, 0,  33);

  Test_0(&sailable, 0, 90, 0);

  Test_0(&sailable, 0, 120, 0);
  Test_0(&sailable, 0, 180, TackZoneDeg());
  Test_0(&sailable, 0, 179, TackZoneDeg() - 1);
  Test_0(&sailable, 0, -179, (TackZoneDeg() + 1));  // still sticky
  Test_0(&sailable, 0, -175, (TackZoneDeg() + 5));  // still sticky
  Test_0(&sailable, 0, -170, (TackZoneDeg() + 10));  // still sticky
  Test_0(&sailable, 0, -165, (TackZoneDeg() + 15));  // still sticky
  Test_0(&sailable, 0, -160, -TackZoneDeg() + 20);  // flipped
  Test_0(&sailable, 0, -155, -TackZoneDeg() + 25);
  Test_0(&sailable, 0, -150, -TackZoneDeg() + 30);
  Test_0(&sailable, 50, 0, 50);
  Test_0(&sailable, 90, 0, 90);
  Test_0(&sailable, 91, 0, 91);
  Test_0(&sailable, -50, 0, -50);
  Test_0(&sailable, -90, 0, -90);
  Test_0(&sailable, -91, 0, -91);
}

typedef struct Env {
  double alpha_true;
  double alpha_app;
  double mag_app;
  double phi_z;
  double delta_app;  // difference of apparent wind in relation to true wind
} EnvT;

namespace {
void Test(double* sailable, double star, const EnvT& env, double expected) {
  printf("Want to sail %lf at %lf true wind.\n", star, Rad2Deg(env.alpha_true));
  Sector sector;
  double target;
  *sailable = SailableHeading(Deg2Rad(star),
                              env.alpha_true,
                              env.alpha_app + env.delta_app,
                              env.mag_app,
                              env.phi_z,
                              *sailable,
                              &sector,
                              &target);
  EXPECT_FLOAT_EQ(Deg2Rad(expected), *sailable);
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

  printf("Env: T%lf A%lf delta:%lf |A%lf| phi_z%lf\n",
         env->alpha_true, env->alpha_app, env->delta_app,
         env->mag_app, env->phi_z);
}

}  // namespace

TEST(PolarDiagramTest, SailableHeading) {
  double sailable = 0;
  Env env = {0, 0, 10, 0, 0};  // running

  SetEnv(0, 10, 0, 2, &env);  // Go North, running.

  const double JibeZoneWidth = 180 - JibeZoneDeg();
  Sector sector;
  double target;
  sailable = SailableHeading(Deg2Rad(1),
                              env.alpha_true,
                              env.alpha_app,
                              env.mag_app,
                              env.phi_z,
                              sailable,
                              &sector,
                              &target);
  EXPECT_FLOAT_EQ( Deg2Rad(JibeZoneWidth), sailable);

  //              a*         expected
  Test(&sailable, 45, env,   45);
  Test(&sailable,  0, env,   JibeZoneWidth);
  // Hysteresis effect (stick to the previous decision if the difference is not too big)
  Test(&sailable,  -1, env,  JibeZoneWidth);
  Test(&sailable,  -5, env,  JibeZoneWidth);
  Test(&sailable,  -8, env,  JibeZoneWidth);
  Test(&sailable,  -9, env, -JibeZoneWidth); // flipped over
  Test(&sailable, -10, env, -JibeZoneWidth);
  Test(&sailable, -JibeZoneWidth, env, -JibeZoneWidth);
  Test(&sailable, -30, env, -30);
  Test(&sailable,  -9, env, -JibeZoneWidth);
  Test(&sailable,  -5, env, -JibeZoneWidth);
  Test(&sailable,  -1, env, -JibeZoneWidth);
  Test(&sailable,   0, env, -JibeZoneWidth);
  Test(&sailable,   1, env, -JibeZoneWidth);
  Test(&sailable,   8, env, -JibeZoneWidth);
  Test(&sailable,   9, env,  JibeZoneWidth);
  Test(&sailable,  15, env,  JibeZoneWidth);
  Test(&sailable,  33, env,  33);

  Test(&sailable,  50, env,  50);
  Test(&sailable,  90, env,  90);
  Test(&sailable,  91, env,  91);
  Test(&sailable, -50, env, -50);
  Test(&sailable, -90, env, -90);
  Test(&sailable, -91, env, -91);

  SetEnv(90, 10, 0, 2, &env);   // Still go North, Reaching.
  Test(&sailable, 0, env, 0);
  SetEnv(120, 10, 0, 2, &env);  // Still go North, Reaching.
  Test(&sailable, 0, env, 0);

  SetEnv(120, 10, 0, 2, &env);
  Test(&sailable, 0, env, 0);
  SetEnv(180, 10, 0, 2, &env);
  Test(&sailable, 0.00001, env, TackZoneDeg());
  // for boat speed 0 the apparent wind direction is equal to the true wind direction.
  double boat_speed = 0;
  SetEnv(179, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() - 1);
  SetEnv(-179, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 1);  // still sticky
  SetEnv(-175, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 5);  // still sticky
  SetEnv(-170, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 10);  // still sticky
  SetEnv(-165, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 15);  // still sticky
  SetEnv(-160, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 20);  // flipped
  SetEnv(-155, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 25);
  SetEnv(-150, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 30);
  SetEnv(90, 10, 0, boat_speed, &env);
  printf("AA");
  Test(&sailable, 60, env, 60);

  // So the wind turned right by 3 degrees
  env.delta_app = Deg2Rad(3);
  // But the TackZone is reduced for the apparent wind by more than 3 degree.
  SetEnv(179, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() - 1);
  printf("HH");
  SetEnv(-179, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 1);  // still sticky
  printf("HH");
  SetEnv(-175, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 5);  // still sticky
  printf("HH");
  SetEnv(-170, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 10);  // still sticky
  SetEnv(-165, 10, 0, boat_speed, &env);
  printf("HH");
  Test(&sailable, 0, env, TackZoneDeg() + 15);  // still sticky
  SetEnv(-160, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 20);  // flipped
  printf("HH");
  SetEnv(-155, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 25);
  SetEnv(-150, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 30);
  SetEnv(150, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() - 30);

  printf("The other way round\n\n");

  SetEnv(179, 10, 0, boat_speed, &env);
  env.delta_app = Deg2Rad(-3);  // So the wind turned left by 3 degrees

  SetEnv(179, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() - 1);
  SetEnv(-179, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 1);  // still sticky
  SetEnv(-175, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 5);  // still sticky
  SetEnv(-170, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 10);  // still sticky
  SetEnv(-165, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 15);  // still sticky
  SetEnv(-160, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 20);  // flipped
  SetEnv(-155, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 25);
  SetEnv(-150, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 30);
  SetEnv(150, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() - 30);

  boat_speed = 2.5;
  env.delta_app = Deg2Rad(0);             // Stable wind
  SetEnv(120, 10, 0, boat_speed, &env);   // Still go North, Reaching.
  Test(&sailable, 0, env, 0);
  SetEnv(180, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg());
  SetEnv(179, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() - 1);
  SetEnv(-179, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 1);  // still sticky
  SetEnv(-175, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 5);  // still sticky
  SetEnv(-170, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 10);  // still sticky
  SetEnv(-165, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() + 15);  // still sticky
  SetEnv(-160, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 20);  // flipped
  SetEnv(-155, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 25);
  SetEnv(-150, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, -TackZoneDeg() + 30);
  SetEnv(150, 10, 0, boat_speed, &env);
  Test(&sailable, 0, env, TackZoneDeg() - 30);

}







int main(int argc, char* argv[]) {
  PolarDiagramTest_All();
  PolarDiagramTest_BestSailable();
  PolarDiagramTest_BestStableSailableHeading();
  PolarDiagramTest_SailableHeading();
  return 0;
}
