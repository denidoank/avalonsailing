// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "helmsman/filter_block.h"

#include "common/convert.h"
#include "common/normalize.h"
#include "common/polar.h"
#include "common/unknown.h"
#include "helmsman/apparent.h"
#include "helmsman/boat.h"   // kWindSensorOffsetRad
#include "helmsman/controller_io.h"
#include "lib/testing/testing.h"

void SetEnv(const Polar& wind_true,
            const Polar& boat,
            double gamma_sail,
            ControllerInput* in) {
  // The sensor sees the differnce of true global wind vector and
  // boat speed vector.
  Polar sensor_wind = wind_true - boat;

  double angle_sensor =
      sensor_wind.AngleRad() // But the sensor is
      - kWindSensorOffsetRad // mounted on the mast not straight but turned left,
      - gamma_sail         // the mast turns with the sail,
      - boat.AngleRad()    // the sail turns with the boat AND
      - M_PI;              // it gives the direction where it comes from (not goes to as any other motion vector.

  // The effects of turning the mast on the magnitude are neglected.
  double mag_sensor = sensor_wind.Mag();

  in->wind.mag_kn = MeterPerSecondToKnots(mag_sensor);
  in->wind.alpha_deg = Rad2Deg(NormalizeRad(angle_sensor));

  in->imu.speed_m_s = boat.Mag();
  in->imu.attitude.phi_z_rad = boat.AngleRad();
  
  in->drives.gamma_sail_rad = gamma_sail;
}

TEST(FilterBlock, All) {
  FilterBlock b;
  ControllerInput in;
  FilteredMeasurements filtered;
  //SkipperInput out;

  in.Reset();
  filtered.Reset();
  // All angles in radians
  EXPECT_FLOAT_EQ(0, in.drives.gamma_sail_rad);
  EXPECT_EQ(false, in.drives.homed_sail);

  EXPECT_FLOAT_EQ(0, filtered.phi_z_boat);
  EXPECT_FLOAT_EQ(0, filtered.mag_boat);
  EXPECT_FLOAT_EQ(0, filtered.omega_boat);
  EXPECT_FLOAT_EQ(0, filtered.alpha_true);
  EXPECT_FLOAT_EQ(0, filtered.mag_true);
  EXPECT_FLOAT_EQ(0, filtered.angle_app);
  EXPECT_FLOAT_EQ(0, filtered.mag_app);
  EXPECT_FLOAT_EQ(0, filtered.angle_sensor);
  EXPECT_FLOAT_EQ(0, filtered.mag_sensor);
  EXPECT_FLOAT_EQ(0, filtered.longitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.latitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.phi_x_rad);
  EXPECT_FLOAT_EQ(0, filtered.phi_y_rad);
  EXPECT_FLOAT_EQ(0, filtered.temperature_c);

  b.Filter(in, &filtered);
  EXPECT_FLOAT_EQ(0, filtered.phi_z_boat);
  EXPECT_FLOAT_EQ(0, filtered.mag_boat);
  EXPECT_FLOAT_EQ(0, filtered.omega_boat);
  EXPECT_FLOAT_EQ(kUnknown, filtered.alpha_true);
  EXPECT_FLOAT_EQ(kUnknown, filtered.mag_true);
  EXPECT_FLOAT_EQ(kUnknown, filtered.angle_app);
  EXPECT_FLOAT_EQ(0, filtered.mag_app);
  EXPECT_FLOAT_EQ(0, filtered.angle_sensor);
  EXPECT_FLOAT_EQ(0, filtered.mag_sensor);
  EXPECT_FLOAT_EQ(0, filtered.longitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.latitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.phi_x_rad);
  EXPECT_FLOAT_EQ(0, filtered.phi_y_rad);
  EXPECT_FLOAT_EQ(0, filtered.temperature_c);
  EXPECT_EQ(false, filtered.valid);

  // The time until all filters are valid depends on the filter constants and
  // the sampling period, but eventually after a few second, the filters should
  // be filled and valid.
  for (int i = 0; i < 20; ++i)
    b.Filter(in, &filtered);
  EXPECT_FLOAT_EQ(0, filtered.phi_z_boat);
  EXPECT_FLOAT_EQ(0, filtered.mag_boat);
  EXPECT_FLOAT_EQ(0, filtered.omega_boat);
  EXPECT_FLOAT_EQ(kUnknown, filtered.alpha_true);
  EXPECT_FLOAT_EQ(kUnknown, filtered.mag_true);
  EXPECT_FLOAT_EQ(kUnknown, filtered.angle_app);
  EXPECT_FLOAT_EQ(0, filtered.mag_app);
  EXPECT_FLOAT_EQ(0, filtered.angle_sensor);
  EXPECT_FLOAT_EQ(0, filtered.mag_sensor);
  EXPECT_FLOAT_EQ(0, filtered.longitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.latitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.phi_x_rad);
  EXPECT_FLOAT_EQ(0, filtered.phi_y_rad);
  EXPECT_FLOAT_EQ(20, filtered.temperature_c);
  EXPECT_FLOAT_EQ(true, filtered.valid);

  // Everything in rad here.
  Polar wind_true(0, 2);  // The wind vector forward to North, with 2m/s.
  Polar boat(0, 1);       // The boat is going forward as well, with 1 m/s,
                          // so the apparent wind vector is at 0 degree to
                          // the boats x-axis, 1m/s magnitude.
  double gamma_sail = Deg2Rad(0);
  SetEnv(wind_true, boat, gamma_sail, &in);
  for (int i = 0; i < 20; ++i) {
    b.Filter(in, &filtered);
    // printf("%6.4f\n", filtered.mag_app);
  }
  EXPECT_FLOAT_EQ(0, filtered.phi_z_boat);
  EXPECT_FLOAT_EQ(1, filtered.mag_boat);
  EXPECT_FLOAT_EQ(0, filtered.omega_boat);
  EXPECT_FLOAT_EQ(kUnknown, filtered.alpha_true);
  EXPECT_FLOAT_EQ(kUnknown, filtered.mag_true);
  EXPECT_FLOAT_EQ(kUnknown, filtered.angle_app);
  EXPECT_FLOAT_EQ(1.0, filtered.mag_app);
  EXPECT_FLOAT_EQ(M_PI - kWindSensorOffsetRad, filtered.angle_sensor);
  EXPECT_FLOAT_EQ(1.0, filtered.mag_sensor);
  EXPECT_FLOAT_EQ(0, filtered.longitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.latitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.phi_x_rad);
  EXPECT_FLOAT_EQ(0, filtered.phi_y_rad);
  EXPECT_FLOAT_EQ(20, filtered.temperature_c);
  EXPECT_EQ(true, filtered.valid);

  // With sail angle information we can calculate the true wind.
  in.drives.homed_sail = true;
  b.Filter(in, &filtered);
  EXPECT_FLOAT_EQ(0, filtered.angle_app);
  EXPECT_FLOAT_EQ(1.0, filtered.mag_app);
  EXPECT_FLOAT_EQ(M_PI - kWindSensorOffsetRad, filtered.angle_sensor);
  EXPECT_FLOAT_EQ(1.0, filtered.mag_sensor);
  EXPECT_EQ(false, b.ValidTrueWind());  // Must fail now!!
  // slow filter for true wind
  for (int i = 0; i < 2000; ++i) {
    b.Filter(in, &filtered);
    //printf("true alpha: %6.4f mag:%6.4f\n",
    //       filtered.alpha_true, filtered.mag_true);
  }
  EXPECT_FLOAT_EQ(0.0, filtered.alpha_true);
  EXPECT_FLOAT_EQ(2.0, filtered.mag_true);
  EXPECT_EQ(true, b.ValidTrueWind());
  
  wind_true = Polar(-M_PI / 2, 2);  // wind vector to West, with 2m/s
  boat = Polar(0, 2);               // boat going North, with 2 m/s
  // so the apparent wind vector is at -135 degree to
  // the boats x-axis, sqrt(2) * 2m/s magnitude.
  SetEnv(wind_true, boat, gamma_sail, &in);
  printf("From East wind\n");
  for (int i = 0; i < 20; ++i) {
    b.Filter(in, &filtered);
    printf("app: %6.4f %6.4f true: %6.4f\n",
           filtered.mag_app, filtered.angle_app, filtered.mag_true);
  }
  EXPECT_FLOAT_EQ(-M_PI * 3.0 / 4, filtered.angle_app);
  EXPECT_FLOAT_EQ(2.0 * sqrt(2), filtered.mag_app);
  EXPECT_FLOAT_EQ(-M_PI * 3.0 / 4  + M_PI - kWindSensorOffsetRad - gamma_sail,
                  filtered.angle_sensor);
  EXPECT_FLOAT_EQ(2.0 * sqrt(2), filtered.mag_sensor);
  // 1000 ticks for the 100s average period + 5 ticks for the median-of-5 filter
  for (int i = 0; i < 1005 + 20; ++i)
    b.Filter(in, &filtered);
  EXPECT_FLOAT_EQ(-M_PI / 2, filtered.alpha_true);
  EXPECT_FLOAT_EQ(2.0, filtered.mag_true);

  // With gamma sail we expect ...
  gamma_sail = 0.1;
  SetEnv(wind_true, boat, gamma_sail, &in);
  for (int i = 0; i < 20; ++i) {
    b.Filter(in, &filtered);
  }
  // no effect on true and apparent wind ...
  EXPECT_IN_INTERVAL(-M_PI / 2 - 0.001, filtered.alpha_true, -M_PI / 2 + 0.001);
  EXPECT_IN_INTERVAL(1.998, filtered.mag_true, 2.002);
  EXPECT_FLOAT_EQ(-M_PI * 3.0 / 4, filtered.angle_app);
  EXPECT_FLOAT_EQ(2.0 * sqrt(2), filtered.mag_app);
  // but on the wind sensor direction.
  EXPECT_FLOAT_EQ(-M_PI * 3.0 / 4  + M_PI - kWindSensorOffsetRad - gamma_sail,
                  filtered.angle_sensor);
  EXPECT_FLOAT_EQ(2.0 * sqrt(2), filtered.mag_sensor);
}


int main(int argc, char* argv[]) {
  FilterBlock_All();
  return 0;
}

