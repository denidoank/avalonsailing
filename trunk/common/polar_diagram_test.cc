// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
#include "common/polar_diagram.h"

#include <math.h>
#include <stdio.h>
#include "common/convert.h"
#include "lib/testing/testing.h"

// print polar diagram as x-y-chart.
// Columns: angle/degree, speed_magnitude/kt, speed_x/kt, speed_y/kt,
// tack_zone, jibe_zone,
// and xy values for constant speed circles at the 50%, 75% and 100% of the
// wind speed.
void LogPolarDiagram() {
  for (double wind_speed = 5; wind_speed <= 10; wind_speed += 5) {
    printf("\n For wind_speed: %g knots\n", wind_speed);
    printf("angle/degree,speed_magnitude/kt,speed_x/kt,speed_y/kt,"
           "tack_zone,jibe_zone,"
           "wind_speed_50_x/kt,wind_speed_50_y/kt,"
           "wind_speed_75_x/kt,wind_speed_75_y/kt,"
           "wind_speed_100_x/kt,wind_speed_100_y/kt\n");
    for (double alpha = -180.0; alpha <= 180; alpha += 2) {
      double rad = Deg2Rad(alpha);
      bool dead_zone_tack;
      bool dead_zone_jibe;
      double boat_speed;
      ReadPolarDiagram(alpha,
                       wind_speed,
                       &dead_zone_tack,
                       &dead_zone_jibe,
                       &boat_speed);
      printf("%g,%g,%g,%g,%d,%d,%g,%g,%g,%g,%g,%g\n",
             alpha, boat_speed, boat_speed * sin(rad), boat_speed * cos(rad),
             dead_zone_tack ? 1 : 0, dead_zone_jibe ? 1 : 0,
             0.5 * wind_speed * sin(rad), 0.5 * wind_speed * cos(rad),
             0.75 * wind_speed * sin(rad), 0.75 * wind_speed * cos(rad),
             1.0 * wind_speed * sin(rad), 1.0 * wind_speed * cos(rad));
      printf("%g,%g,\n", alpha, boat_speed);
    }
  }
}

TEST(PolarDiagramTest, All) {
double wind_speed = 10;

bool dead_zone_tack;
bool dead_zone_jibe;
double close_reach_speed;

double tack_zone_angle = TackZoneDeg();
ReadPolarDiagram(tack_zone_angle,
                 wind_speed,
                 &dead_zone_tack,
                 &dead_zone_jibe,
                 &close_reach_speed);
EXPECT_EQ(dead_zone_tack, false);
EXPECT_EQ(dead_zone_jibe, false);

// speed against the wind
double boat_speed;
ReadPolarDiagram(0,
                 wind_speed,
                 &dead_zone_tack,
                 &dead_zone_jibe,
                 &boat_speed);

EXPECT_EQ(dead_zone_tack, true);
EXPECT_EQ(dead_zone_jibe, false);

EXPECT_FLOAT_EQ(3.539395, boat_speed);
EXPECT_FLOAT_EQ(cos(Deg2Rad(tack_zone_angle)) * close_reach_speed,
                boat_speed);


double jibe_zone_angle = JibeZoneDeg();
double jibe_zone_speed;
ReadPolarDiagram(jibe_zone_angle,
                 wind_speed,
                 &dead_zone_tack,
                 &dead_zone_jibe,
                 &jibe_zone_speed);
EXPECT_EQ(dead_zone_tack, false);
EXPECT_EQ(dead_zone_jibe, false);

// speed with the wind
ReadPolarDiagram(180,
                 wind_speed,
                 &dead_zone_tack,
                 &dead_zone_jibe,
                 &boat_speed);

EXPECT_EQ(dead_zone_tack, false);
EXPECT_EQ(dead_zone_jibe, true);

EXPECT_FLOAT_EQ(5.4464617, boat_speed);
EXPECT_FLOAT_EQ(fabs(cos(Deg2Rad(jibe_zone_angle))) * jibe_zone_speed,
                boat_speed);

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

int main(int argc, char* argv[]) {
  PolarDiagramTest_All();
  PolarDiagramTest_BestSailable();
  return 0;
}
