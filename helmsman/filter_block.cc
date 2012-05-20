// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

// What shall be filtered?
// The IMU measurements are outputs of a Kalman-Bucy-filter and as such
// they are filtered with an unknown delay dependant on the noise.
// After IMU tests we found out that the speed data are just a very rough
// approximation of the real motion vector over ground. The bearing (phi_z)
// and the rotational speeds omega_z need spike suppression and sliding average filters of 1s and 8s respectively.
// The drive actual values are exact and noise-free.
// The wind sensor updates once per second and reflects the strong
// fluctuations of the measured wind overlayed with the errors
// by the boats motion (masttip, x- and y-axis-rotation of the boat, caused by waves).
// Consequently filtering is applied only for:
//   1. The true wind to get a very calm general wind direction (averaged at least over 60s).
//   2. The heel angle calculated from the acceleration vector.
//

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

const double kOmegaZFilterPeriod = 8.0;
const double kSpeedFilterPeriod = 60.0;

}  // namespace

extern int debug;

bool ValidGPS(const ControllerInput& in) {
  return !isnan(in.imu.position.longitude_deg) && !isnan(in.imu.position.latitude_deg) &&
         !(in.imu.position.longitude_deg == 0 && in.imu.position.latitude_deg == 0);
}


FilterBlock::FilterBlock()
  : valid_app_wind_(false),
    imu_fault_(false),
    gps_fault_(false),
    len_1s_(1.0 / kSamplingPeriod),
    len_30s_(30.0 / kSamplingPeriod),
    len_100s_(100.0 / kSamplingPeriod),     // 100s, N.B. The ship control state Initial cannot be
                                            // shorter than this time period.

    om_z_filter_(kOmegaZFilterPeriod / kSamplingPeriod),
    speed_filter_(kSpeedFilterPeriod / kSamplingPeriod),

    mag_app_filter_(len_1s_),
    mag_true_filter_(len_100s_),
    mag_aoa_filter_(len_30s_),

    phi_z_filter_(),
    phi_z_wrap_(&phi_z_filter_),
    angle_app_filter_(len_1s_),
    angle_app_wrap_(&angle_app_filter_),
    alpha_true_filter_(len_100s_),
    alpha_true_wrap_(&alpha_true_filter_),
    angle_aoa_filter_(len_30s_),
    angle_aoa_wrap_(&angle_aoa_filter_) {}


// Clip magnitude at the maximum possible boat speed.
double FilterBlock::CensorSpeed(double speed_in) {
  const double clip_speed = 2.8;  // m/s
  if (speed_in < -clip_speed)
    return -clip_speed;
  if (speed_in > clip_speed)
    return clip_speed;
  return speed_in;
}


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
// 5. GPS fault, no time, no position

// TODO: if a filter input becomes invalid, it invalidates the filter state.
// The filter needs a reset and some time to recover.
void FilterBlock::Filter(const ControllerInput& in,
                         FilteredMeasurements* fil) {
  fil->Reset();
  // TODO: signal mixer (IMU + compass) comes here. It overwrites IMU data.
  imu_fault_ = isnan(in.imu.attitude.phi_z_rad) || isnan(in.imu.velocity.x_m_s);
  if (imu_fault_)
    fprintf(stderr, "IMU fault\n");
  gps_fault_ = !ValidGPS(in);
  if (gps_fault_)
    fprintf(stderr, "GPS fault\n");

  // yaw (== bearing) from 2 independant sources: IMU Kalman filter, raw IMU magnetic sensor and
  // independant compass.
  double consensus = 0;
  double compass_phi_z = compass_mixer_.Mix(in.imu.attitude.phi_z_rad, imu_fault_ ? 1 : 0,
                                            in.imu.compass.phi_z_rad, in.imu.compass.valid ? 1 : 0,
                                            in.compass_sensor.phi_z_rad, 1,  // invalid if outdated TODO
                                            &consensus);
  if (consensus >= 0.5) {
    fil->phi_z_boat = phi_z_wrap_.Filter(compass_phi_z);
    if (debug) {
      fprintf(stderr, "filtered compass phi_z %lg deg\n", Rad2Deg(fil->phi_z_boat));
    }
  } else {
    fprintf(stderr, "No compass consensus, %lf\n", consensus);
  }

  // NaNs irreversibly poison filters, so we have to prevent that.
  // With the GPS data we are not that picky about temporary faults.
  // TODO: If GPS is missing for 10 minutes, then we should log this and
  // the Skipper should get an info that this is the last known position, not the current one.
  if (!gps_fault_) {
    ASSIGN_NOT_NAN(fil->longitude_deg, in.imu.position.longitude_deg);  // GPS-Data
    ASSIGN_NOT_NAN(fil->latitude_deg,  in.imu.position.latitude_deg);
  }

  // If everything else fails we optimistically assume that we are making some speed forward.
  fil->mag_boat = 1;
  if (!imu_fault_) {
    ASSIGN_NOT_NAN(fil->phi_x_rad, in.imu.attitude.phi_x_rad);      // roll angle, heel;
    ASSIGN_NOT_NAN(fil->phi_y_rad, in.imu.attitude.phi_y_rad);      // pitch, nick angle;

    fil->mag_boat = CensorSpeed(speed_filter_.Filter(in.imu.velocity.x_m_s));
  }

  double om_z = 0;
  ASSIGN_NOT_NAN(om_z, in.imu.gyro.omega_z_rad_s);                 // yaw rate, rotational speed around z axis
  fil->omega_boat = om_z_filter_.Filter(om_z_med_.Filter(om_z));
  if (debug && false) {
    fprintf(stderr, "raw om_z %lg filtered %lg\n", om_z, fil->omega_boat);
  }

  ASSIGN_NOT_NAN(fil->temperature_c, in.imu.temperature_c);           // in deg Celsius

  fprintf(stderr, "filter in:%c homed: %c sensor: %lg %lg\n", in.wind_sensor.valid ? 'V' : 'I', in.drives.homed_sail ? 'V' : 'I', in.wind_sensor.alpha_deg, in.wind_sensor.mag_m_s);

  // The wind sensor updates the wind once per second.
  double alpha_wind_rad = NormalizeRad(Deg2Rad(in.wind_sensor.alpha_deg));
  double mag_wind_m_s = in.wind_sensor.mag_m_s;

  fil->angle_aoa = SymmetricRad(angle_aoa_wrap_.Filter(SymmetricRad(alpha_wind_rad + kWindSensorOffsetRad)));
  fil->mag_aoa = mag_aoa_filter_.Filter(mag_wind_m_s);


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
                Polar(fil->phi_z_boat, fil->mag_boat),
                fil->phi_z_boat,
                &wind_true);
      fil->alpha_true = SymmetricRad(alpha_true_wrap_.Filter(NormalizeRad(wind_true.AngleRad())));
      fil->mag_true = mag_true_filter_.Filter(wind_true.Mag());
      if (debug)
        fprintf(stderr, "trueInOut alpha mag: %lg, %lg, app: %lg, %lg\n", wind_true.AngleRad(), wind_true.Mag(), fil->alpha_true, fil->mag_true);
      // The validity of the true wind takes some time and is flagged separately with
      // valid_true_wind.
    } else {
      if (debug)
        fprintf(stderr, "Imu failed -> no true wind.");
    }
    fil->angle_app = SymmetricRad(angle_app_wrap_.Filter(angle_app));
    fil->mag_app =  mag_app_filter_.Filter(mag_app);
    valid_app_wind_ = angle_app_wrap_.ValidOutput() && mag_app_filter_.ValidOutput();
  } else {
    fil->alpha_true = kUnknown;  // TODO check that all later processing does not stumble over this.
    fil->mag_true = kUnknown;
    fil->angle_app = kUnknown;
    fil->mag_app =  mag_wind_m_s;
    valid_app_wind_ = false;
    if (debug && !in.drives.homed_sail)
      fprintf(stderr, "!in.drives.homed_sail");
    if (debug && !in.wind_sensor.valid)
      fprintf(stderr, "!in.wind_sensor.valid");

  }
  // All but the true wind.
  fil->valid = valid_app_wind_ && om_z_filter_.ValidOutput() &&
               om_z_med_.ValidOutput() && phi_z_wrap_.ValidOutput();

  fil->valid_app_wind = valid_app_wind_;
  fil->valid_true_wind = ValidTrueWind();

  if (debug)
    fprintf(stderr, "%c true: %lg %lg, app: %c %lg %lg \n",
            fil->valid_true_wind ? 'V' : 'I', fil->alpha_true, fil->mag_true,
            valid_app_wind_ ?  'V' : 'I', fil->angle_app,  fil->mag_app);
}


bool FilterBlock::ValidAppWind() {
  return valid_app_wind_;
}

bool FilterBlock::ValidTrueWind() {
  return alpha_true_wrap_.ValidOutput() && mag_true_filter_.ValidOutput() && !imu_fault_ && valid_app_wind_;
}

bool FilterBlock::ValidSpeed() {
  return speed_filter_.ValidOutput() && !imu_fault_;
}
