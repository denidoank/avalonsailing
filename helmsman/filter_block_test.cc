// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "helmsman/filter_block.h"

#include <math.h>

#include "common/convert.h"
#include "common/normalize.h"
#include "common/polar.h"
#include "common/unknown.h"
#include "helmsman/apparent.h"
#include "helmsman/boat.h"   // kWindSensorOffsetRad
#include "helmsman/controller_io.h"
#include "helmsman/sampling_period.h"
#include "lib/testing/testing.h"

const static double kCool = 12;

void SetEnv(const Polar& wind_true,
            const Polar& boat,
            double gamma_sail,
            ControllerInput* in) {
  // The sensor sees the difference of true global wind vector and
  // boat speed vector.
  Polar sensor_wind = wind_true - boat;

  double angle_sensor =
      sensor_wind.AngleRad() // But the sensor is
      - kWindSensorOffsetRad // mounted on the mast not straight but turned left,
      - gamma_sail           // the mast turns with the sail,
      - boat.AngleRad()      // the sail turns with the boat AND
      - M_PI;           // It gives the direction where it comes from (not where
                        // it blows *to* as any other motion vector).

  // The effects of turning the mast on the magnitude are neglected.
  double mag_sensor = sensor_wind.Mag();

  in->wind_sensor.mag_m_s = mag_sensor;
  in->wind_sensor.alpha_deg = Rad2Deg(NormalizeRad(angle_sensor));
  in->wind_sensor.valid = true;

  in->imu.velocity.x_m_s = boat.Mag();
  in->imu.velocity.y_m_s = 0;
  in->imu.attitude.phi_z_rad = boat.AngleRad();
  in->imu.temperature_c = kCool;

  in->drives.gamma_sail_rad = gamma_sail;
}

TEST(FilterBlock, All) {
  FilterBlock b;
  ControllerInput in;
  FilteredMeasurements filtered;
  int calls_until_valid = 0;
  int calls_until_wind_valid = 0;
  int calls_until_speed_valid = 0;

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
  EXPECT_FLOAT_EQ(0, filtered.angle_aoa);
  EXPECT_FLOAT_EQ(0, filtered.mag_aoa);
  EXPECT_FLOAT_EQ(0, filtered.longitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.latitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.phi_x_rad);
  EXPECT_FLOAT_EQ(0, filtered.phi_y_rad);
  EXPECT_FLOAT_EQ(0, filtered.temperature_c);
  EXPECT_FALSE(b.ValidTrueWind());
  EXPECT_FALSE(b.ValidSpeed());
  EXPECT_FALSE(ValidGPS(in));

  b.Filter(in, 0, &filtered);
  calls_until_valid += !filtered.valid;
  calls_until_wind_valid += !b.ValidTrueWind();
  calls_until_speed_valid += !b.ValidSpeed();
  EXPECT_FLOAT_EQ(0, filtered.phi_z_boat);
  EXPECT_FLOAT_EQ(0, filtered.mag_boat);
  EXPECT_FLOAT_EQ(0, filtered.omega_boat);
  EXPECT_FLOAT_EQ(kUnknown, filtered.alpha_true);
  EXPECT_FLOAT_EQ(kUnknown, filtered.mag_true);
  EXPECT_FLOAT_EQ(kUnknown, filtered.angle_app);
  EXPECT_FLOAT_EQ(0, filtered.mag_app);
  EXPECT_FLOAT_EQ(0, filtered.angle_aoa);
  EXPECT_FLOAT_EQ(0, filtered.mag_aoa);
  EXPECT_FLOAT_EQ(0, filtered.longitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.latitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.phi_x_rad);
  EXPECT_FLOAT_EQ(0, filtered.phi_y_rad);
  EXPECT_FLOAT_EQ(20, filtered.temperature_c);
  EXPECT_EQ(false, filtered.valid);
  EXPECT_FALSE(b.ValidTrueWind());
  EXPECT_FALSE(b.ValidSpeed());
  EXPECT_FALSE(ValidGPS(in));

  // The time until all filters are valid depends on the filter constants and
  // the sampling period, but eventually after a few second, the filters should
  // be filled and valid.
  // The sail drive needs to be ready first (because we
  // cannot know the wind direction otherwise).
  // All drives get ready.
  in.drives.homed_sail = true;
  in.drives.homed_rudder_left = true;
  in.drives.homed_rudder_right = true;
  for (int i = 0; i < 2.0 / kSamplingPeriod; ++i) {
    b.Filter(in, 0, &filtered);
    calls_until_valid += !filtered.valid;
    calls_until_wind_valid += !b.ValidTrueWind();
    calls_until_speed_valid += !b.ValidSpeed();
  }
  EXPECT_FLOAT_EQ(0, filtered.phi_z_boat);
  EXPECT_FLOAT_EQ(0, filtered.mag_boat);
  EXPECT_FLOAT_EQ(0, filtered.omega_boat);
  EXPECT_FLOAT_EQ(kUnknown, filtered.alpha_true);
  EXPECT_FLOAT_EQ(kUnknown, filtered.mag_true);
  EXPECT_FLOAT_EQ(kUnknown, filtered.angle_app);
  EXPECT_FLOAT_EQ(0, filtered.mag_app);
  EXPECT_FLOAT_EQ(0, filtered.angle_aoa);
  EXPECT_FLOAT_EQ(0, filtered.mag_aoa);
  EXPECT_FLOAT_EQ(0, filtered.longitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.latitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.phi_x_rad);
  EXPECT_FLOAT_EQ(0, filtered.phi_y_rad);
  EXPECT_FLOAT_EQ(20, filtered.temperature_c);
  EXPECT_FALSE(filtered.valid);
  EXPECT_FALSE(b.ValidTrueWind());
  EXPECT_FALSE(b.ValidSpeed());
  EXPECT_FALSE(ValidGPS(in));

  // The wind sensor has to report valid in order to
  // calculate the apparent wind.
  in.wind_sensor.valid = true;

  for (int i = 0; i < 2.0 / kSamplingPeriod; ++i) {
    b.Filter(in, 0, &filtered);
    calls_until_valid += !filtered.valid;
    calls_until_wind_valid += !b.ValidTrueWind();
    calls_until_speed_valid += !b.ValidSpeed();
  }
  EXPECT_FLOAT_EQ(0, filtered.alpha_true);
  EXPECT_FLOAT_EQ(0, filtered.mag_true);
  EXPECT_FLOAT_EQ(0, filtered.angle_app);
  EXPECT_FLOAT_EQ(0, filtered.mag_app);
  EXPECT_FALSE(filtered.valid);
  EXPECT_FALSE(b.ValidTrueWind());
  EXPECT_FALSE(b.ValidSpeed());

  in.wind_sensor.valid = false;
  b.Filter(in, 0, &filtered);
  calls_until_valid += !filtered.valid;
  calls_until_wind_valid += !b.ValidTrueWind();
  calls_until_speed_valid += !b.ValidSpeed();
  EXPECT_FLOAT_EQ(kUnknown, filtered.angle_app);
  EXPECT_FALSE(filtered.valid);

  in.wind_sensor.valid = true;
  b.Filter(in, 0, &filtered);
  calls_until_valid += !filtered.valid;
  calls_until_wind_valid += !b.ValidTrueWind();
  calls_until_speed_valid += !b.ValidSpeed();
  EXPECT_FALSE(filtered.valid);
  EXPECT_FLOAT_EQ(0, filtered.angle_app);


  // Everything in rad here.
  Polar wind_true(0, 2);  // The wind vector points forward to North, with 2m/s.
  Polar boat(0, 1.0);     // The boat is going forward as well, with 1.5 m/s,
                          // so the apparent wind vector is at 0 degree to
                          // the boats x-axis, 1m/s magnitude.
  double gamma_sail = Deg2Rad(0);
  SetEnv(wind_true, boat, gamma_sail, &in);

  // The time until all filters are valid depends on the filter constants and
  // the sampling period, but eventually after > 100s second, the filters should
  // be filled and even the slow true wind filter has to be valid.
  for (int i = 0; i < 200 / kSamplingPeriod; ++i) {
    b.Filter(in, gamma_sail, &filtered);
    calls_until_valid += !filtered.valid;
    calls_until_wind_valid += !b.ValidTrueWind();
    calls_until_speed_valid += !b.ValidSpeed();
  }

  EXPECT_FLOAT_EQ(0, filtered.phi_z_boat);
  EXPECT_LT(0, filtered.mag_boat);
  EXPECT_FLOAT_EQ(0, filtered.omega_boat);
  EXPECT_FLOAT_EQ(0, filtered.angle_app);
  EXPECT_FLOAT_EQ(1, filtered.mag_app);
  EXPECT_LT(fabs(SymmetricRad(-M_PI - filtered.angle_aoa)), 0.000001);
  EXPECT_FLOAT_EQ(1, filtered.mag_aoa);
  EXPECT_FLOAT_EQ(0, filtered.longitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.latitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.phi_x_rad);
  EXPECT_FLOAT_EQ(0, filtered.phi_y_rad);
  EXPECT_FLOAT_EQ(kCool, filtered.temperature_c);
  EXPECT_TRUE(filtered.valid);
  EXPECT_TRUE(b.ValidTrueWind());
  EXPECT_TRUE(b.ValidSpeed());
  // EXPECT_FLOAT_EQ(0, filtered.alpha_true); both are undefined here
  // EXPECT_FLOAT_EQ(2, filtered.mag_true);

  for (int i = 0; i < 2.0 / kSamplingPeriod; ++i) {
    b.Filter(in, gamma_sail, &filtered);
    // printf("%6.4lf\n", filtered.mag_app);
    calls_until_valid += !filtered.valid;
    calls_until_wind_valid += !b.ValidTrueWind();
    calls_until_speed_valid += !b.ValidSpeed();
  }
  EXPECT_FLOAT_EQ(0, filtered.phi_z_boat);
  EXPECT_FLOAT_EQ(1, filtered.mag_boat);
  EXPECT_FLOAT_EQ(0, filtered.omega_boat);
  EXPECT_FLOAT_EQ(0, filtered.alpha_true);
  EXPECT_FLOAT_EQ(2, filtered.mag_true);
  EXPECT_FLOAT_EQ(0, filtered.angle_app);
  EXPECT_FLOAT_EQ(1, filtered.mag_app);
  EXPECT_LT(fabs(SymmetricRad(M_PI - kWindSensorOffsetRad - filtered.angle_aoa)), 0.000001);
  EXPECT_FLOAT_EQ(1.0, filtered.mag_aoa);
  EXPECT_FLOAT_EQ(0, filtered.longitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.latitude_deg);
  EXPECT_FLOAT_EQ(0, filtered.phi_x_rad);
  EXPECT_FLOAT_EQ(0, filtered.phi_y_rad);
  EXPECT_FLOAT_EQ(kCool, filtered.temperature_c);
  EXPECT_TRUE(filtered.valid);
  EXPECT_TRUE(b.ValidTrueWind());
  EXPECT_TRUE(b.ValidSpeed());

  EXPECT_EQ(int(7.9 / kSamplingPeriod + 0.5), calls_until_valid);
  EXPECT_EQ(int(102.1 / kSamplingPeriod + 0.5), calls_until_wind_valid);
  EXPECT_EQ(int(19.9 / kSamplingPeriod + 0.5), calls_until_speed_valid);


  // With sail angle information we can calculate the true wind.
  in.drives.homed_sail = true;
  b.Filter(in, gamma_sail, &filtered);
  EXPECT_FLOAT_EQ(0, filtered.angle_app);
  EXPECT_FLOAT_EQ(1.0, filtered.mag_app);
  EXPECT_LT(fabs(SymmetricRad(M_PI - kWindSensorOffsetRad - filtered.angle_aoa)), 0.000001);
  EXPECT_FLOAT_EQ(1.0, filtered.mag_aoa);
  EXPECT_EQ(true, b.ValidTrueWind());  // Must fail now!!
  // slow filter for true wind
  for (int i = 0; i < 2000; ++i) {
    b.Filter(in, gamma_sail, &filtered);
    //printf("true alpha: %6.4lf mag:%6.4lf\n",
    //       filtered.alpha_true, filtered.mag_true);
  }
  EXPECT_FLOAT_EQ(0.0, filtered.alpha_true);
  EXPECT_FLOAT_EQ(2.0, filtered.mag_true);
  EXPECT_EQ(true, b.ValidTrueWind());

  // Increase the speed and allow the slow speed filter to settle.
  boat = Polar(0, 2);               // boat going North, with 2 m/s
  // So the apparent wind vector is at -135 degree to
  // the boats x-axis, sqrt(2) * 2m/s magnitude.
  SetEnv(wind_true, boat, gamma_sail, &in);
  for (int i = 0; i < 120 / kSamplingPeriod; ++i) {
    b.Filter(in, gamma_sail, &filtered);
  }
  EXPECT_FLOAT_EQ(2, filtered.mag_boat);

  wind_true = Polar(-M_PI / 2, 2);  // Wind vector into the West, with 2m/s,
  boat = Polar(0, 2);               // the boat going North, with 2 m/s.
  // So the apparent wind vector is at -135 degree to
  // the boats x-axis, sqrt(2) * 2m/s magnitude.
  SetEnv(wind_true, boat, gamma_sail, &in);
  printf("From East wind\n");
  for (int i = 0; i < 10 / kSamplingPeriod; ++i) {
    b.Filter(in, gamma_sail, &filtered);
    printf("app: %6.4lf %6.4lf true: %6.4lf\n",
           filtered.mag_app, filtered.angle_app, filtered.mag_true);
  }
  EXPECT_FLOAT_EQ(-M_PI * 3.0 / 4, filtered.angle_app);
  EXPECT_FLOAT_EQ(2.0 * sqrt(2), filtered.mag_app);
  // 1000 ticks for the 100s average period + 5 ticks for the median-of-5 filter
  for (int i = 0; i < 110 / kSamplingPeriod; ++i)
    b.Filter(in, gamma_sail, &filtered);
  // true wind and aoa are slowly filtered, so we have to wait a bit.
  EXPECT_FLOAT_EQ(M_PI / 4 - kWindSensorOffsetRad - gamma_sail,
                  filtered.angle_aoa);
  EXPECT_FLOAT_EQ(2.0 * sqrt(2), filtered.mag_aoa);
  EXPECT_FLOAT_EQ(-M_PI / 2, filtered.alpha_true);
  EXPECT_FLOAT_EQ(2.0, filtered.mag_true);

  // With gamma sail we expect ...
  gamma_sail = 0.1;
  SetEnv(wind_true, boat, gamma_sail, &in);
  // Apparent wind filter has 4s.
  for (int i = 0; i < 45; ++i) {
    b.Filter(in, gamma_sail, &filtered);
  }
  // no effect on true and apparent wind ...
  EXPECT_IN_INTERVAL(-M_PI / 2 - 0.001, filtered.alpha_true, -M_PI / 2 + 0.001);
  EXPECT_IN_INTERVAL(1.998, filtered.mag_true, 2.002);
  EXPECT_IN_INTERVAL(-M_PI * 3.0 / 4 - 0.002, filtered.angle_app, -M_PI * 3.0 / 4 + 0.002);
  EXPECT_FLOAT_EQ(2.0 * sqrt(2), filtered.mag_app);
  // but on the wind sensor direction.
  for (int i = 0; i < 110 / kSamplingPeriod; ++i)
    b.Filter(in, gamma_sail, &filtered);
  EXPECT_FLOAT_EQ(M_PI / 4  - kWindSensorOffsetRad - gamma_sail,
                  filtered.angle_aoa);
  EXPECT_FLOAT_EQ(2.0 * sqrt(2), filtered.mag_aoa);
}

int main(int argc, char* argv[]) {
  FilterBlock_All();
  return 0;
}
