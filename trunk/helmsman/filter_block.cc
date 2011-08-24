// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/filter_block.h"

#include <stdio.h>
#include "helmsman/apparent.h"
#include "helmsman/boat.h"  // kWindSensorOffsetRad
#include "helmsman/controller_io.h"
#include "helmsman/sampling_period.h"

#include "common/check.h"
#include "common/normalize.h"
#include "common/polar.h"
#include "common/unknown.h"

namespace {
struct FilterElement {
  FilterInterface* filter;
  const double* in;
  double* out;
};

const double kShortFilterPeriod = 1.0; // s
const double kLongFilterPeriod = 100.0; // s, N.B. The state Initial cannot be
                                        // shorter than this time period.
}  // namespace


void FilterBlock::Filter(const ControllerInput& in,
                         FilteredMeasurements* fil) {
  double zz[kChannels];
  double mag_wind_m_s = in.wind_sensor.mag_m_s;
  double alpha_wind_rad = NormalizeRad(Deg2Rad(in.wind_sensor.alpha_deg));

  // The wind sensor is telling where the wind is coming *from*, but we work
  // with motion vectors pointing here the wind is going *to*.
  double angle_sail_raw = (alpha_wind_rad - M_PI + kWindSensorOffsetRad);

  const FilterElement in_block[kChannels] = {
      {median_ + 0,  &in.imu.speed_m_s,              zz + 0},  // in m/s, x-component of speed vector
      {median_ + 1,  &in.imu.position.longitude_deg, zz + 1},  // GPS-Data
      {median_ + 2,  &in.imu.position.latitude_deg,  zz + 2},
      {median_ + 3,  &in.imu.attitude.phi_x_rad,     zz + 3},  // roll; IMU-Data
      {median_ + 4,  &in.imu.attitude.phi_y_rad,     zz + 4},  // pitch;
      {&wrap_med_1_, &in.imu.attitude.phi_z_rad,     zz + 5},  // yaw;
      {median_ + 6,  &in.imu.gyro.omega_z_rad_s,     zz + 6},  // yaw rate
      {median_ + 7,  &in.imu.temperature_c,          zz + 7},  // in deg Celsius
      {median_ + 8,  &mag_wind_m_s,                  zz + 8},  // true wind magnitude
      {&wrap_med_2_, &alpha_wind_rad,                zz + 9}}; // true wind vector direction
  CHECK_EQ(kChannels, sizeof(in_block) / sizeof(in_block[0]));
  bool valid = true;
  for (int i = 0; i < kChannels; ++i) {
    CHECK_NE(in_block[i].filter, NULL);
    if (isnan(*in_block[i].in)) {
      fprintf(stderr, "Filter input channel %d has NaN data. Skipping\n", i);
      continue;
    }  
    *in_block[i].out = in_block[i].filter->Filter(*in_block[i].in);
    valid = valid && in_block[i].filter->ValidOutput();
  }

  zz[5] = NormalizeRad(zz[5]);
  zz[9] = NormalizeRad(zz[9]);

  FilterElement out_block[] = {
      {&average_0_, zz + 0, &fil->mag_boat},      // in m/s
      {&average_1_, zz + 1, &fil->longitude_deg}, // GPS-Data
      {&average_2_, zz + 2, &fil->latitude_deg},
      {&average_3_, zz + 3, &fil->phi_x_rad},     // roll; IMU-Data
      {&average_4_, zz + 4, &fil->phi_y_rad},     // pitch;
      {&wrap_1_,    zz + 5, &fil->phi_z_boat},    // yaw;
      {&average_6_, zz + 6, &fil->omega_boat},
      {&average_7_, zz + 7, &fil->temperature_c}, // in deg C
      {&average_8_, zz + 8, &fil->mag_sensor},    // Wind
      {&wrap_2_,    zz + 9, &fil->angle_sensor}};
  CHECK_EQ(kChannels, sizeof(out_block) / sizeof(out_block[0]));
  for (int i = 0; i < kChannels; ++i) {
    CHECK_NE(out_block[i].filter, NULL);
    *out_block[i].out = out_block[i].filter->Filter(*out_block[i].in);
    valid = valid && out_block[i].filter->ValidOutput();
  }

  if (in.drives.homed_sail) {
    fil->angle_app = SymmetricRad(angle_sail_raw + in.drives.gamma_sail_rad);
    fil->mag_app = fil->mag_sensor;

    Polar wind_true(0, 0);
    TruePolar(Polar(fil->angle_app,  fil->mag_app),
              Polar(fil->phi_z_boat, fil->mag_boat),
              fil->phi_z_boat,
              &wind_true);

    fil->mag_true = average_mag_true_.Filter(
        median_mag_true_.Filter(wind_true.Mag()));
    fil->alpha_true = SymmetricRad(wrap_3_.Filter(
        wrap_med_3_.Filter(NormalizeRad(wind_true.AngleRad()))));
  } else {
    fil->angle_app = kUnknown;
    fil->mag_app = fil->mag_sensor;
    fil->alpha_true = kUnknown;  // TODO check that all later processing does not stumble over this.
    fil->mag_true = kUnknown;
  }
  fil->valid = valid;
}

FilterBlock::FilterBlock()
  : initial_(true),
    len_short_(kShortFilterPeriod / kSamplingPeriod),
    len_long_(kLongFilterPeriod / kSamplingPeriod),
    average_0_(len_short_),
    average_1_(len_short_),
    average_2_(len_short_),
    average_3_(len_short_),
    average_4_(len_short_),
    average_5_(len_short_),
    average_6_(len_short_),
    average_7_(len_short_),
    average_8_(len_short_),
    average_9_(len_short_),
    average_mag_true_(len_long_),
    wrap_med_1_(&median_wr_1_),
    wrap_med_2_(&median_wr_2_),
    wrap_med_3_(&median_wr_3_),
    average_wr_1_(len_short_),
    average_wr_2_(len_short_),
    average_wr_3_(len_long_),
    wrap_1_(&average_wr_1_),
    wrap_2_(&average_wr_2_),
    wrap_3_(&average_wr_3_) {}


bool FilterBlock::ValidTrueWind() {
  return wrap_3_.ValidOutput();
}
