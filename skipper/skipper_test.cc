// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "skipper/skipper.h"

#include <vector>
#include <stdlib.h>

#include "common/convert.h"
#include "common/normalize.h"
#include "common/unknown.h"
#include "lib/testing/testing.h"
#include "skipper/lat_lon.h"
#include "skipper/target_circle.h"
#include "skipper/plans.h"


const static double kDays = 3600 * 24;

TEST(Skipper, All) {
  double end_time = 0;
  SkipperInput in;
  std::vector<AISInfo> ais;
  double alpha_star;

  // no wind, default direction
  Skipper::Run(in, ais, &alpha_star);
  EXPECT_FLOAT_EQ(225, alpha_star);
  in.mag_true_kn = kUnknown;
  in.angle_true_deg = kUnknown;
  Skipper::Run(in, ais, &alpha_star);
  EXPECT_FLOAT_EQ(225, alpha_star);
  in.mag_true_kn = 2;
  in.angle_true_deg = 0;  // so we cannot sail south

  // Thalwil test, with south wind
  double x0 = 47.2962-0.008;
  double y0 = 8.5812-0.008;   // 1000m SW of the target
  double v = 2 / to_cartesian_meters;  // v is speed in degree/s
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  Skipper::Init(in);
  end_time = 0;

  printf("t/s, x0, y0, alpha_star\n");
  double time_step = 60;
  for (double t = 0; t < 500; t += time_step ) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    Skipper::Run(in, ais, &alpha_star);
    if (!Skipper::TargetReached(LatLon(x0, y0))) {
      EXPECT_FLOAT_EQ(45, alpha_star);
    } else {
      if (end_time == 0)
        end_time = t;
      EXPECT_TRUE(thalwil.In(x0, y0));
    }
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step;
    y0 += v * sin(phi_rad) * time_step;
    printf("%8.6f %8.6f %8.6f %6.4f\n", t, x0, y0, alpha_star);
  }
  printf("end time: %8.6f days.\n", end_time / kDays);
  EXPECT_GT(60 * kDays, end_time);
}

TEST(Skipper, ThalwilOpposingWind) {
  double end_time = 0;
  SkipperInput in;
  std::vector<AISInfo> ais;
  double alpha_star;

  // Thalwil test, with bad opposing wind
  in.angle_true_deg = 225;  // so we cannot sail south
  double x0 = 47.2962-0.008;
  double y0 = 8.5812-0.008;   // 1000m SW of the target
  double v = 2 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  Skipper::Init(in);
  end_time = 0;

  printf("t/s, x0, y0, alpha_star\n");
  double time_step = 60;
  for (double t = 0; t < 5000; t += time_step) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    Skipper::Run(in, ais, &alpha_star);
    if (Skipper::TargetReached(LatLon(x0, y0))) {
      EXPECT_TRUE(thalwil.In(x0, y0));
      if (end_time == 0)
        end_time = t;
    }

    // Simulate the motion
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step;
    y0 += v * sin(phi_rad) * time_step;
    printf("%8.6f %8.6f %8.6f %6.4f\n", t, x0, y0, alpha_star);
  }
  printf("end time: %8.6f days.\n", end_time / kDays);
  EXPECT_GT(60 * kDays, end_time);


}

TEST(Skipper, Atlantic) {
  double end_time = 0;
  SkipperInput in;
  std::vector<AISInfo> ais;
  double alpha_star;
  // Atlantic test, with constantly bad opposing wind
  in.angle_true_deg = 45;  // so we cannot sail south-west directly
  double x0 = 48.2;
  double y0 = -5;
  // speed 2m/s or 4 knots constantly
  double v = 2 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  Skipper::Init(in);
  end_time = 0;

  printf("t/days, x0, y0, alpha_star\n");
  double time_step = 4*3600;
  // need to run this for alonger time.
  for (double t = 0;
       t < 64 * kDays;
       t += time_step ) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    Skipper::Run(in, ais, &alpha_star);

    if (Skipper::TargetReached(LatLon(x0, y0))) {
      EXPECT_TRUE(caribbean_final.In(x0, y0));
      if (end_time == 0)
        end_time = t;
    }

    // Simulate the motion
    time_step = t < 60.5 * kDays ? 4*3600 : 60;
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step;
    y0 += v * sin(phi_rad) * time_step;
    printf("%8.6f %8.6f %8.6f %6.4f\n", t / kDays, x0, y0, alpha_star);
  }
  printf("end time: %8.6f days.\n", end_time / kDays);
  EXPECT_GT(63 * kDays, end_time);

}

TEST(Skipper, ChangingAtlantic) {
  double end_time = 0;
  SkipperInput in;
  std::vector<AISInfo> ais;
  double alpha_star;
  // Atlantic test, with changing winds and erratic storms throwing us off
  // track.
  in.angle_true_deg = 45;
  double x0 = 48.2;
  double y0 = -5;
  // speed 2m/s or 4 knots constantly
  double v = 2 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  Skipper::Init(in);
  end_time = 0;

  printf("t/days, x0, y0, alpha_star\n");
  double time_step = 4*3600;
  // need to run this for alonger time.
  for (double t = 0;
       t < 64 * kDays;
       t += time_step ) {
    in.angle_true_deg = NormalizeDeg(in.angle_true_deg + (rand() % 25));
    in.mag_true_kn = rand() % 40;
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    Skipper::Run(in, ais, &alpha_star);

    if (Skipper::TargetReached(LatLon(x0, y0))) {
      printf(" ");
      EXPECT_TRUE(caribbean_final.In(x0, y0));
      if (end_time == 0)
        end_time = t;
    }

    // Simulate the motion
    time_step = t < 60.5 * kDays ? 4*3600 : 1800;
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step * in.mag_true_kn / 10.0;
    y0 += v * sin(phi_rad) * time_step * in.mag_true_kn / 10.0;
    // Erratic Drift, faster than the boat
    x0 += 5 / to_cartesian_meters * time_step * (rand() % 100 - 50) / 100.0;
    y0 += 5 / to_cartesian_meters * time_step * (rand() % 100 - 50) / 100.0;

    //printf("%8.6f %8.6f %8.6f %6.4f\n", t / kDays, x0, y0, alpha_star);
  }
  printf("end time: %8.6f days.\n", end_time / kDays);
  EXPECT_GT(63 * kDays, end_time);
}

TEST(Skipper, StormyAtlantic) {
  double end_time = 0;
  SkipperInput in;
  std::vector<AISInfo> ais;
  double alpha_star;

  // Stormy Atlantic test, with changing winds and erratic storms throwing us
  // off track.
  in.angle_true_deg = 45;
  double x0 = 48.2;
  double y0 = -5;
  // speed 2m/s or 4 knots constantly
  double v = 2 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  Skipper::Init(in);
  end_time = 0;

  printf("t/days, x0, y0, alpha_star\n");
  double time_step = 4*3600;
  // need to run this for alonger time.
  for (double t = 0;
       t < 100 * kDays;
       t += time_step ) {
    in.angle_true_deg = NormalizeDeg(in.angle_true_deg + (rand() % 25));
    in.mag_true_kn = rand() % 40;
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    Skipper::Run(in, ais, &alpha_star);

    if (Skipper::TargetReached(LatLon(x0, y0))) {
      printf(" ");
      EXPECT_TRUE(caribbean_final.In(x0, y0));
      if (end_time == 0)
        end_time = t;
    }

    // Simulate the motion
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step * in.mag_true_kn / 10.0;
    y0 += v * sin(phi_rad) * time_step * in.mag_true_kn / 10.0;
    // Erratic Drift, faster than the boat
    x0 += 50 / to_cartesian_meters * time_step * (rand() % 100 - 50) / 100.0;
    y0 += 50 / to_cartesian_meters * time_step * (rand() % 100 - 50) / 100.0;

    printf("%8.6f %8.6f %8.6f %6.4f\n", t / kDays, x0, y0, alpha_star);
  }
  // Here the end point is not met (stroms 25 times stronger than the boat)
  // but we get near the target.
}

int main(int argc, char* argv[]) {
  Skipper_All();
  Skipper_ThalwilOpposingWind();
  Skipper_Atlantic();
  Skipper_ChangingAtlantic();
  Skipper_StormyAtlantic();
  return 0;
}
