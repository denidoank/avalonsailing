// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "skipper/skipper_internal.h"

#include <vector>
#include <stdlib.h>

#include "common/convert.h"
#include "common/normalize.h"
#include "common/unknown.h"
#include "lib/testing/testing.h"
#include "skipper/lat_lon.h"
#include "skipper/target_circle.h"
#include "skipper/plans.h"
#include "vskipper/vskipper.h"
#include "vskipper/util.h"

using std::string;

const static double kDays = 3600 * 24;

int debug = 1;

FILE* kml_file = NULL;

static const char KML_head[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<kml xmlns=\"http://www.opengis.net/kml/2.2\">"
  "<Document>"
   " <name>Paths</name>"
    "<description>Avalon Skipper resulting path.</description>"
   " <Style id=\"orangeLineGreenPoly\">"
    "  <LineStyle>"
     "   <color>7f007fff</color> "    // transparency, blue, green, red
      "  <width>4</width>"
     " </LineStyle>"
     " <PolyStyle>"
      "  <color>7f00ff00</color>"
     " </PolyStyle>"
   " </Style>"
   " <Placemark>"
    "  <name>%s</name>"
    "  <description>Avalon path</description>"
    "  <styleUrl>#orangeLineGreenPoly</styleUrl>"
    "  <LineString>"
    "    <extrude>0</extrude>"
    "    <tessellate>1</tessellate>"
    "    <altitudeMode>absolute</altitudeMode>"
    "    <coordinates>";

     // -112.2550785337791,36.07954952145647,2357

static const char KML_tail[] =
    "        </coordinates>"
    "  </LineString>"
    "</Placemark>"
    "</Document>"
    "</kml>";

void OpenKML(const string& name) {
  kml_file = fopen((name + ".kml").c_str(), "w");
  fprintf(kml_file, KML_head, name.c_str());
}

void DotKML(double latitude, double longitude) {
  // 500m altitude is above Lake Zurich, so we get properly tesselated onto the surface.
  fprintf(kml_file, "    %8.6f,%8.6f,500\n", longitude, latitude);
}

void CloseKML() {
  fprintf(kml_file, KML_tail);
  fclose(kml_file);
}


TEST(SkipperInternal, All) {
  fprintf(stderr, "Running test: All");

  double end_time = 0;
  SkipperInput in;
  std::vector<skipper::AisInfo> ais;
  double alpha_star;

  // no wind, default direction
  SkipperInternal::Run(in, ais, &alpha_star);
  EXPECT_FLOAT_EQ(225, alpha_star);
  in.mag_true_kn = kUnknown;
  in.angle_true_deg = kUnknown;
  SkipperInternal::Run(in, ais, &alpha_star);
  EXPECT_FLOAT_EQ(225, alpha_star);
  in.mag_true_kn = 2;
  in.angle_true_deg = 0;  // so we cannot sail south

  // Thalwil test, with south wind
  double x0 = 47.2962-0.008;
  double y0 = 8.5812-0.008;   // 1000m exactly SW of the target
  double v = 2 / to_cartesian_meters;  // v is speed in degree/s
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  SkipperInternal::Init(in);
  end_time = 0;

  printf("t/s, x0, y0, alpha_star\n");
  double time_step = 60;
  for (double t = 0; t < 500; t += time_step ) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    SkipperInternal::Run(in, ais, &alpha_star);
    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
      end_time = t;
      EXPECT_TRUE(thalwil.In(x0, y0));
      break;
    }
    EXPECT_FLOAT_EQ(45, alpha_star);
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step;
    y0 += v * sin(phi_rad) * time_step;
    printf("%8.6f %8.6f %8.6f %6.4f\n", t, x0, y0, alpha_star);
  }
  printf("end time: %8.6f days.\n", end_time / kDays);
  EXPECT_GT(60 * kDays, end_time);
}

TEST(SkipperInternal, SukkulentenhausPlan) {
  fprintf(stderr, "Running test: SukkulentenhausPlan");
  OpenKML("Sukkulentenhaus");
  double end_time = 0;
  SkipperInput in;
  std::vector<skipper::AisInfo> ais;
  double alpha_star;

  in.angle_true_deg = 120;  // so we cannot sail south
  in.mag_true_kn = 2;
  double x0 = 47.3557;
  double y0 = 8.5368;       // Mythenquai, Shore
  double v = 2 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  SkipperInternal::Init(in);
  end_time = 0;

  printf("t/s, x0, y0, alpha_star\n");
  double time_step = 60;
  for (double t = 0; t < 5000; t += time_step) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    SkipperInternal::Run(in, ais, &alpha_star);
    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
      EXPECT_TRUE(sukku.In(x0, y0));
      if (end_time == 0)
        end_time = t;
    }

    // Simulate the motion
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step;
    y0 += v * sin(phi_rad) * time_step;
    printf("%8.6f %8.6f %8.6f %6.4f\n", t, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  printf("end time: %8.6f days.\n", end_time / kDays);
  EXPECT_GT(60 * kDays, end_time);
  CloseKML();
}


TEST(SkipperInternal, ThalwilOpposingWind) {
  fprintf(stderr, "Running test: ThalwilOpposingWind");
  OpenKML("FromThalwilAgainstTheWind");
  double end_time = 0;
  SkipperInput in;
  std::vector<skipper::AisInfo> ais;
  double alpha_star;

  // Thalwil test, with bad opposing wind
  in.angle_true_deg = 225;  // so we cannot sail south
  in.mag_true_kn = 2;
  double x0 = 47.2962-0.008;
  double y0 = 8.5812-0.008;   // 1000m SW of the target
  double v = 2 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  SkipperInternal::Init(in);
  end_time = 0;

  printf("t/s, x0, y0, alpha_star\n");
  double time_step = 60;
  for (double t = 0; t < 5000; t += time_step) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    SkipperInternal::Run(in, ais, &alpha_star);
    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
      EXPECT_TRUE(thalwil.In(x0, y0));
      if (end_time == 0)
        end_time = t;
    }

    // Simulate the motion
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step;
    y0 += v * sin(phi_rad) * time_step;
    printf("%8.6f %8.6f %8.6f %6.4f\n", t, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  printf("end time: %8.6f days.\n", end_time / kDays);
  EXPECT_GT(60 * kDays, end_time);
  CloseKML();
}

TEST(SkipperInternal, Atlantic) {
  fprintf(stderr, "Running test: Atlantic");
  OpenKML("Atlantic");
  double end_time = 0;
  SkipperInput in;
  std::vector<skipper::AisInfo> ais;
  double alpha_star;
  // Atlantic test, with constantly bad opposing wind
  in.angle_true_deg = 45;  // so we cannot sail south-west directly
  in.mag_true_kn = 2;
  double x0 = 48.2;
  double y0 = -5;
  // speed 2m/s or 4 knots constantly
  double v = 2 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  SkipperInternal::Init(in);
  end_time = 0;

  printf("t/days, x0, y0, alpha_star\n");
  double time_step = 4*3600;
  // need to run this for alonger time.
  for (double t = 0;
       t < 64 * kDays;
       t += time_step ) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    SkipperInternal::Run(in, ais, &alpha_star);

    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
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
    DotKML(x0, y0);
  }
  printf("end time: %8.6f days.\n", end_time / kDays);
  EXPECT_GT(63 * kDays, end_time);
  CloseKML();
}

TEST(SkipperInternal, ChangingAtlantic) {
  fprintf(stderr, "Running test: ChangingAtlantic");
  OpenKML("ChangingAtlantic");
  double end_time = 0;
  SkipperInput in;
  std::vector<skipper::AisInfo> ais;
  double alpha_star;
  // Atlantic test, with changing winds and erratic storms throwing us off
  // track.
  in.angle_true_deg = 45;
  in.mag_true_kn = 2;
  double x0 = 48.2;
  double y0 = -5;
  // speed 2m/s or 4 knots constantly
  double v = 2 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  SkipperInternal::Init(in);
  end_time = 0;

  printf("t/days, x0, y0, alpha_star\n");
  // We switch to shorter time steps once it gets interesting.
  double time_step = 4*3600;
  double stop_time = 1E9;
  // need to run this for a longer time. t in seconds.
  for (double t = 0;
       t < 64 * kDays && t < stop_time;
       t += time_step ) {
    in.angle_true_deg = NormalizeDeg(in.angle_true_deg + (rand() % 360));
    in.mag_true_kn = rand() % 40;
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    SkipperInternal::Run(in, ais, &alpha_star);

    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
      printf(" ");
      EXPECT_TRUE(caribbean_final.In(x0, y0));
      if (end_time == 0)
        end_time = t;
      // break;
      stop_time = t + 1800;  // simulate after reaching the target.
    }

    // Simulate the motion
    if (t > 60.5 || y0 < -59 || SkipperInternal::TargetReached(LatLon(x0, y0))) {
      time_step = 600;
    }
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step * in.mag_true_kn / 10.0;
    y0 += v * sin(phi_rad) * time_step * in.mag_true_kn / 10.0;
    // Erratic Drift, faster than the boat
    x0 += 5 / to_cartesian_meters * time_step * (rand() % 100 - 50) / 100.0;
    y0 += 5 / to_cartesian_meters * time_step * (rand() % 100 - 50) / 100.0;

    printf("%8.6f %8.6f %8.6f %6.4f\n", t / kDays, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  printf("end time: %8.6f days.\n", end_time / kDays);
  EXPECT_GT(63 * kDays, end_time);
  CloseKML();
}

TEST(SkipperInternal, StormyAtlantic) {
  fprintf(stderr, "Running test: StormyAtlantic");
  OpenKML("StormsRuleTheWaves");
  double end_time = 0;
  SkipperInput in;
  std::vector<skipper::AisInfo> ais;
  double alpha_star;

  // Stormy Atlantic test, with changing winds and erratic storms throwing us
  // off track.
  in.angle_true_deg = 45;
  in.mag_true_kn = 2;
  double x0 = 48.2;
  double y0 = -5;
  // speed 2m/s or 4 knots constantly
  double v = 2 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  SkipperInternal::Init(in);
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
    SkipperInternal::Run(in, ais, &alpha_star);

    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
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
    DotKML(x0, y0);
  }
  // Here the end point is not met (stroms 25 times stronger than the boat)
  // but we get near the target.
  CloseKML();
}

int main(int argc, char* argv[]) {
  //SkipperInternal_All();
  //SkipperInternal_SukkulentenhausPlan();
  //SkipperInternal_ThalwilOpposingWind();
  //SkipperInternal_Atlantic();
  SkipperInternal_ChangingAtlantic();
  //SkipperInternal_StormyAtlantic();
  return 0;
}
