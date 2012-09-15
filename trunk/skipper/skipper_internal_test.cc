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
  SkipperInternal::Run(in, ais, &alpha_star, NULL);
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


TEST(SkipperInternal, ToulonPlan) {
  fprintf(stderr, "Running test: ToulonPlan\n");
  OpenKML("Toulon");
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
  for (double t = 0; t < 600000; t += time_step) {
    in.latitude_deg = x0;
    in.longitude_deg = y0;
    SkipperInternal::Run(in, ais, &alpha_star, NULL);
    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
      EXPECT_TRUE(toulon_target.In(x0, y0));
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

TEST(SkipperInternal, ToulonDetailsPlan) {
  fprintf(stderr, "Running test: ToulonDetailsPlan\n");
  OpenKML("ToulonTarget");
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
    SkipperInternal::Run(in, ais, &alpha_star, NULL);
    if (SkipperInternal::TargetReached(LatLon(x0, y0))) {
      EXPECT_TRUE(toulon_target.In(x0, y0));
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


int main(int argc, char* argv[]) {
  SkipperInternal_Storm();
  SkipperInternal_ToulonPlan();
  SkipperInternal_ToulonDetailsPlan();
  return 0;
}
