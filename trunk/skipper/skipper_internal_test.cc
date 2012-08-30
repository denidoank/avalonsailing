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
const static double meter2degree = 1.0 / 111111.1; // from degrees to meters distance, 1 degree is 111.1km at the equator.

int debug = 1;

FILE* kml_file = NULL;

static const char KML_head[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
  "<Document>\n"
   " <name>Paths</name>\n"
    "<description>Avalon Skipper resulting path.</description>\n"
   " <Style id=\"orangeLineGreenPoly\">\n"
    "  <LineStyle>\n"
     "   <color>7f007fff</color>\n"    // transparency, blue, green, red
      "  <width>4</width>\n"
     " </LineStyle>\n"
     " <PolyStyle>\n"
      "  <color>7f00ff00</color>\n"
     " </PolyStyle>\n"
   " </Style>\n"
   " <Placemark>\n"
    "  <name>%s</name>\n"
    "  <description>Avalon path</description>\n"
    "  <styleUrl>#orangeLineGreenPoly</styleUrl>\n"
    "  <LineString>\n"
    "    <extrude>0</extrude>\n"
    "    <tessellate>1</tessellate>\n"
    "    <altitudeMode>absolute</altitudeMode>\n"
    "    <coordinates>\n";

// -112.2550785337791,36.07954952145647,2357

static const char KML_tail[] =
    "        </coordinates>\n"
    "  </LineString>\n"
    "</Placemark>\n"
    "</Document>\n"
    "</kml>\n";

void OpenKML(const string& name) {
  kml_file = fopen((name + ".kml").c_str(), "w");
  fprintf(kml_file, KML_head, name.c_str());
}

void DotKML(double latitude, double longitude) {
  // The altitude should be above Lake Zurich (406), so we get properly tesselated onto the surface.
  // The sea is at height 0m, Captain Obvious.
  fprintf(kml_file, "    %8.6lf,%8.6lf,410\n", longitude, latitude);
}

void CloseKML() {
  fprintf(kml_file, KML_tail);
  fclose(kml_file);
}


TEST(SkipperInternal, Storm) {
  SkipperInput in;
  std::vector<skipper::AisInfo> ais;
  double alpha_star;
  in.mag_true_kn = kUnknown;
  in.angle_true_deg = kUnknown;
  SkipperInternal::Run(in, ais, &alpha_star);
  EXPECT_FLOAT_EQ(225, alpha_star);
  in.mag_true_kn = 40;

  double angle_true_deg = 2;

  WindStrengthRange wind_strength = kNormalWind;
  double planned = 33;
  double storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                       angle_true_deg,
                                                       planned);
  EXPECT_FLOAT_EQ(33, storm_override);

  angle_true_deg = 0;
  storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                angle_true_deg,
                                                planned);
  EXPECT_FLOAT_EQ(33, storm_override);

  wind_strength = kStormWind;
  storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                angle_true_deg,
                                                planned);
  const double storm_angle = 50;
  EXPECT_FLOAT_EQ(storm_angle, storm_override);

  planned = -33;
  storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                angle_true_deg,
                                                planned);
  EXPECT_FLOAT_EQ(storm_angle, storm_override);

  wind_strength = kNormalWind;
  storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                angle_true_deg,
                                                planned);
  EXPECT_FLOAT_EQ(-33, storm_override);

  planned = -34;
  storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                angle_true_deg,
                                                planned);
  EXPECT_FLOAT_EQ(-34, storm_override);

  wind_strength = kStormWind;
  storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                angle_true_deg,
                                                planned);
  EXPECT_FLOAT_EQ(360 - storm_angle, storm_override);

  planned = 44;
  storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                angle_true_deg,
                                                planned);
  EXPECT_FLOAT_EQ(360 - storm_angle, storm_override);

  // GPS fault
  in.latitude_deg = NAN;
  in.longitude_deg = NAN    ;
  storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                angle_true_deg,
                                                planned);
  EXPECT_FLOAT_EQ(360 - storm_angle, storm_override);

  wind_strength = kNormalWind;
  storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                angle_true_deg,
                                                planned);
  EXPECT_FLOAT_EQ(44, storm_override);

  wind_strength = kStormWind;
  storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                angle_true_deg,
                                                planned);
  EXPECT_FLOAT_EQ(storm_angle, storm_override);
  // Now the true wind rotates. We follow that rotation.
  for (int i = 0; i < 720; ++i) {
    storm_override = SkipperInternal::HandleStorm(wind_strength,
                                                  SymmetricDeg(angle_true_deg + i),
                                                  planned);
    EXPECT_FLOAT_EQ(NormalizeDeg(storm_angle + i), storm_override);
  }


}



TEST(SkipperInternal, Thalwil) {
  fprintf(stderr, "Running test: Thalwil\n");
  OpenKML("Thalwil");

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
  // So we have about 90degrees to the wind when we go NorthEast.
  in.angle_true_deg = -45;

  // Thalwil test, with South East wind
  double x0 = 47.2962 - 0.005;
  double y0 = 8.5812 - 0.005 / cos(Deg2Rad(x0));   // 800m exactly SW of the target
  // v is speed in degree/s, longitude needs divisor cos(latitude).
  double v = 2 * meter2degree;

  in.latitude_deg = x0;
  in.longitude_deg = y0;
  SkipperInternal::Init(in);
  end_time = 0;

  // check that a circle is round
  double time_step = 50;  // seconds
  for (double t = 0; t <= 360; t += 10) {
    double phi_rad = Deg2Rad(t);
    // expect a radius of 2m/s * timestep = 100m
    double x = x0 + v * cos(phi_rad) * time_step;
    double y = y0 + v * sin(phi_rad) * time_step / cos(Deg2Rad(x0));
    printf("%8.6lf %8.6lf %8.6lf %6.4lf\n", t, x, y, alpha_star);
    DotKML(x, y);
  }

  printf("t/s, x0, y0, alpha_star\n");
  for (double t = 0; t < 500; t += time_step ) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    SkipperInternal::Run(in, ais, &alpha_star);
    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
      end_time = t;
      EXPECT_TRUE(thalwil.In(x0, y0));
      break;
    }
    EXPECT_IN_INTERVAL(44.99, alpha_star, 45.01);
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step;
    y0 += v * sin(phi_rad) * time_step / cos(Deg2Rad(x0));
    printf("%8.6lf %8.6lf %8.6lf %6.4lf\n", t, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  printf("end time: %8.6lf days.\n", end_time / kDays);
  CloseKML();
  EXPECT_GT(0.03 * kDays, end_time);
}

TEST(SkipperInternal, SukkulentenhausPlan) {
  fprintf(stderr, "Running test: SukkulentenhausPlan\n");
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
    y0 += v * sin(phi_rad) * time_step / cos(Deg2Rad(x0));
    printf("%8.6lf %8.6lf %8.6lf %6.4lf\n", t, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  printf("end time: %8.6lf days.\n", end_time / kDays);
  EXPECT_GT(60 * kDays, end_time);
  CloseKML();
}

TEST(SkipperInternal, UfenauPlan) {
  fprintf(stderr, "Running test: UfenauPlan\n");
  OpenKML("Ufenau");
  double end_time = 0;
  SkipperInput in;
  std::vector<skipper::AisInfo> ais;
  double alpha_star;

  in.angle_true_deg = 320;  // so we cannot sail south directly
  in.mag_true_kn = 10;
  double x0 = 47.3557;
  double y0 = 8.5368;       // Mythenquai, Shore
  double v = 2 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  SkipperInternal::Init(in);
  end_time = 0;

  printf("t/s, x0, y0, alpha_star\n");
  double time_step = 60;
  for (double t = 0; t < 15000; t += time_step) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    SkipperInternal::Run(in, ais, &alpha_star);
    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
      EXPECT_TRUE(ufenau_target.In(x0, y0));
      if (end_time == 0)
        end_time = t;
    }

    // Simulate the motion
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step;
    y0 += v * sin(phi_rad) * time_step / cos(Deg2Rad(x0));
    printf("%8.6lf %8.6lf %8.6lf %6.4lf\n", t, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  printf("end time: %8.6lf days.\n", end_time / kDays);
  EXPECT_GT(1 * kDays, end_time);
  CloseKML();
}

TEST(SkipperInternal, MallorcaPlan) {
  fprintf(stderr, "Running test: MallorcaPlan\n");
  OpenKML("Mallorca");
  double end_time = 0;
  SkipperInput in;
  std::vector<skipper::AisInfo> ais;
  double alpha_star;

  in.angle_true_deg = 20;  // so we cannot sail south directly
  in.mag_true_kn = 15;
  double x0 = 43.0617;
  double y0 = 6.0967;       // off Toulon, NOT in the first toulon target circle!
  double v = 1.6 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  SkipperInternal::Init(in);
  end_time = 0;

  printf("t/s, x0, y0, alpha_star\n");
  double time_step = 60;
  for (double t = 0; t < 450000; t += time_step) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    SkipperInternal::Run(in, ais, &alpha_star);
    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
      EXPECT_TRUE(mallorca_target.In(x0, y0));
      if (end_time == 0)
        end_time = t;
    }

    // Simulate the motion
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step;
    y0 += v * sin(phi_rad) * time_step / cos(Deg2Rad(x0));
    printf("%8.6lf %8.6lf %8.6lf %6.4lf\n", t, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  printf("end time: %8.6lf days.\n", end_time / kDays);
  EXPECT_GT(5 * kDays, end_time);
  CloseKML();
}

TEST(SkipperInternal, MallorcaDetailsPlan) {
  fprintf(stderr, "Running test: MallorcaDetailsPlan\n");
  OpenKML("MallorcaTarget");
  double end_time = 0;
  SkipperInput in;
  std::vector<skipper::AisInfo> ais;
  double alpha_star;

  in.angle_true_deg = 20;  // so we cannot sail south directly
  in.mag_true_kn = 15;
  double x0 = 39.937896;
  double y0 = 3.326506;       // very near to the target
  double v = 1.6 / to_cartesian_meters;
  in.latitude_deg = x0;
  in.longitude_deg = y0;
  SkipperInternal::Init(in);
  end_time = 0;

  printf("t/s, x0, y0, alpha_star\n");
  double time_step = 120;
  for (double t = 0; t < 15000; t += time_step) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    SkipperInternal::Run(in, ais, &alpha_star);
    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
      EXPECT_TRUE(mallorca_target.In(x0, y0));
      if (end_time == 0)
        end_time = t;
    }

    // Simulate the motion
    double phi_rad = Deg2Rad(alpha_star);
    x0 += v * cos(phi_rad) * time_step;
    y0 += v * sin(phi_rad) * time_step / cos(Deg2Rad(x0));
    printf("%8.6lf %8.6lf %8.6lf %6.4lf\n", t, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  printf("end time: %8.6lf days.\n", end_time / kDays);
  EXPECT_GT(0.5 * kDays, end_time);
  CloseKML();
}

TEST(SkipperInternal, ThalwilOpposingWind) {
  fprintf(stderr, "Running test: ThalwilOpposingWind\n");
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
    y0 += v * sin(phi_rad) * time_step / cos(Deg2Rad(x0));
    printf("%8.6lf %8.6lf %8.6lf %6.4lf\n", t, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  printf("end time: %8.6lf days.\n", end_time / kDays);
  EXPECT_GT(60 * kDays, end_time);
  CloseKML();
}

TEST(SkipperInternal, Atlantic) {
  fprintf(stderr, "Running test: Atlantic\n");
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
  // need to run this for a longer time.
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
    y0 += v * sin(phi_rad) * time_step / cos(Deg2Rad(x0));
    y0 = SymmetricDeg(y0);
    printf("%8.6lf %8.6lf %8.6lf %6.4lf\n", t / kDays, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  printf("end time: %8.6lf days.\n", end_time / kDays);
  EXPECT_GT(63 * kDays, end_time);
  CloseKML();
}

TEST(SkipperInternal, ChangingAtlantic) {
  fprintf(stderr, "Running test: ChangingAtlantic\n");
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
    y0 += v * sin(phi_rad) * time_step * in.mag_true_kn / 10.0 / cos(Deg2Rad(x0));
    // Erratic Drift, faster than the boat
    x0 += 5 / to_cartesian_meters * time_step * (rand() % 100 - 50) / 100.0;
    y0 += 5 / to_cartesian_meters * time_step * (rand() % 100 - 50) / 100.0 / cos(Deg2Rad(x0));
    y0 = SymmetricDeg(y0);
    printf("%8.6lf %8.6lf %8.6lf %6.4lf\n", t / kDays, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  printf("end time: %8.6lf days.\n", end_time / kDays);
  EXPECT_GT(63 * kDays, end_time);
  CloseKML();
}

TEST(SkipperInternal, StormyAtlantic) {
  fprintf(stderr, "Running test: StormyAtlantic\n");
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
    y0 += v * sin(phi_rad) * time_step * in.mag_true_kn / 10.0 / cos(Deg2Rad(x0));
    // Erratic Drift, faster than the boat. Magnitude 50 blows us to the North pole.
    x0 += 40 / to_cartesian_meters * time_step * (rand() % 100 - 50) / 100.0;
    y0 += 40 / to_cartesian_meters * time_step * (rand() % 100 - 50) / 100.0 / cos(Deg2Rad(x0));
    y0 = SymmetricDeg(y0);

    printf("%8.6lf %8.6lf %8.6lf %6.4lf\n", t / kDays, x0, y0, alpha_star);
    DotKML(x0, y0);
  }
  // Here the end point is not met (The stroms are 25 times stronger than the boat)
  // but we approach the target.
  CloseKML();
}

int main(int argc, char* argv[]) {
  SkipperInternal_Storm();
  SkipperInternal_MallorcaPlan();
  SkipperInternal_MallorcaDetailsPlan();
  SkipperInternal_UfenauPlan();
  SkipperInternal_Thalwil();
  SkipperInternal_SukkulentenhausPlan();
  SkipperInternal_ThalwilOpposingWind();
  SkipperInternal_Atlantic();
  SkipperInternal_ChangingAtlantic();
  SkipperInternal_StormyAtlantic();
  return 0;
}
