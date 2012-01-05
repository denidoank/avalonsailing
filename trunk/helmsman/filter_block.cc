// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

// What shall be filtered?
// The IMU measurements are outputs of a Kalman-Bucy-filter and as such
// they are filtered with an unknown delay dependant on the noise.
// The drive actual values are exact and noise-free.
// The wind sensor updates once per second and reflects the strong
// fluctuations of the measured wind overlayed with the errors
// by the boats motion (masttip, x- and y-axis-rotation, caused by waves).
// Consequently filtering is applied only for:
//   1. The true wind to get a very calm general wind direction (averaged at least over 60s).
//   2. The heel angle calculated from the acceleration vector.

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

const double kShortFilterPeriod = 1.0;   // s
const double kMiddleFilterPeriod = 30.0;
const double kLongFilterPeriod = 100.0;  // s, N.B. The state Initial cannot be
                                         // shorter than this time period.
static int debug = 0;
}  // namespace

#define ASSIGN_NOT_NAN(lhs, rhs) \
  if (isnan(rhs)) { \
    fprintf(stderr, "Filter input " #rhs " has NaN data. Skipping\n"); \
  } else { \
    lhs = rhs; \
  }


// What can go wrong with our inputs:
// 1. sail drive fault,-> not homed - no apparent wind direction (mag is available) -> no true wind
// 2. sail drive position offset -> offset in all wind directions and angles
// 3. wind sensor fault -> valid flag is false
// 4. IMU fault -> no speed nor direction -> no true wind


void FilterBlock::Filter(const ControllerInput& in,
                         FilteredMeasurements* fil) {
  // NaNs irreversibly poison a filter, so have to prevent that.
  ASSIGN_NOT_NAN(fil->mag_boat,      in.imu.speed_m_s);               // in m/s, x-component of speed vector
  ASSIGN_NOT_NAN(fil->longitude_deg, in.imu.position.longitude_deg);  // GPS-Data
  ASSIGN_NOT_NAN(fil->latitude_deg,  in.imu.position.latitude_deg);
  ASSIGN_NOT_NAN(fil->phi_x_rad,     in.imu.attitude.phi_x_rad);      // roll angle, heel;
  ASSIGN_NOT_NAN(fil->phi_y_rad,     in.imu.attitude.phi_y_rad);      // pitch, nick angle;
  ASSIGN_NOT_NAN(fil->phi_z_boat,    in.imu.attitude.phi_z_rad);      // yaw, bearing;
  ASSIGN_NOT_NAN(fil->omega_boat,    in.imu.gyro.omega_z_rad_s);      // yaw rate, rotational speed around z axis
  ASSIGN_NOT_NAN(fil->temperature_c, in.imu.temperature_c);           // in deg Celsius

  // fprintf(stderr, "filter in:%c homed: %c sensor: %g %g\n", in.wind_sensor.valid ? 'V' : 'I', in.drives.homed_sail ? 'V' : 'I', in.wind_sensor.alpha_deg, in.wind_sensor.mag_m_s);

  // The wind sensor updates the wind once per second.
  double alpha_wind_rad = NormalizeRad(Deg2Rad(in.wind_sensor.alpha_deg));
  double mag_wind_m_s = in.wind_sensor.mag_m_s;

  fil->angle_aoa = SymmetricRad(wrap_middle_av_aoa_.Filter(SymmetricRad(alpha_wind_rad + kWindSensorOffsetRad)));
  fil->mag_aoa = av_middle_aoa_.Filter(mag_wind_m_s);

  imu_fault_ = isnan(in.imu.attitude.phi_z_rad) || isnan(in.imu.speed_m_s);
  if (imu_fault_)
    fprintf(stderr, "imu NaN, IMU fault\n");

  if (in.drives.homed_sail && in.wind_sensor.valid) {
    // The wind sensor is telling where the wind is coming *from*, but we work
    // with motion vectors pointing here the wind is going *to*, that is why we have M_PI here.
    // kWindSensorOffsetRad reflects the mounting inaccuracies of mast, mast top unit and sensor,
    // it should be small.
    double angle_app = SymmetricRad(alpha_wind_rad - M_PI + kWindSensorOffsetRad + in.drives.gamma_sail_rad);
    double mag_app = mag_wind_m_s;
    if (mag_app == 0)
      angle_app = 0;

    if (!imu_fault_) {
      Polar wind_true(0, 0);
      TruePolar(Polar(angle_app,  mag_app),
                Polar(fil->phi_z_boat, fil->mag_boat),  // could consider drift here, but y-speed is flaky.
                fil->phi_z_boat,
                &wind_true);
      fil->alpha_true = SymmetricRad(wrap_long_av_.Filter(NormalizeRad(wind_true.AngleRad())));
      fil->mag_true = av_long_.Filter(wind_true.Mag());
      if (debug)
        fprintf(stderr, "trueInOut %g, %g, %g, %g\n", wind_true.AngleRad(), wind_true.Mag(), fil->alpha_true, fil->mag_true);
      // The validity of the true wind takes some time and is flagged separately with
      // valid_true_wind.
    }
    fil->angle_app = SymmetricRad(wrap_short_av_.Filter(angle_app));
    fil->mag_app =  av_short_.Filter(mag_app);
    valid_ =  wrap_short_av_.ValidOutput() && av_short_.ValidOutput();
  } else {
    fil->alpha_true = kUnknown;  // TODO check that all later processing does not stumble over this.
    fil->mag_true = kUnknown;
    fil->angle_app = kUnknown;
    fil->mag_app =  mag_wind_m_s;
    valid_ = false;
  }
  fil->valid = valid_;
  fil->valid_true_wind = ValidTrueWind();

  // fprintf(stderr, "%c true: %g %g, app: %g %g \n", valid_?'V':'I', fil->alpha_true, fil->mag_true, fil->angle_app, fil->mag_app);
}

FilterBlock::FilterBlock()
  : valid_(false),
    imu_fault_(false),
    len_short_(kShortFilterPeriod / kSamplingPeriod),
    len_middle_(kMiddleFilterPeriod / kSamplingPeriod),
    len_long_(kLongFilterPeriod / kSamplingPeriod),
    av_short_(len_short_),
    av_middle_(len_middle_),
    av_long_(len_long_),
    av_middle_aoa_(len_middle_),
    average_short_(len_short_),
    average_long_(len_long_),
    average_middle_aoa_(len_middle_),
    wrap_short_av_(&average_short_),
    wrap_long_av_(&average_long_),
    wrap_middle_av_aoa_(&average_middle_aoa_) {}

bool FilterBlock::ValidTrueWind() {
  return wrap_long_av_.ValidOutput() && av_long_.ValidOutput() && !imu_fault_ && valid_;
}
