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
#include "helmsman/boat.h"  // kWindSensorOffsetRad, kOmegaMaxSail
#include "helmsman/controller_io.h"
#include "helmsman/sampling_period.h"

#include "common/check.h"
#include "common/delta_angle.h"
#include "common/limit_rate.h"
#include "common/normalize.h"
#include "common/polar.h"
#include "common/unknown.h"


int logging_aoa = 1;

namespace {

const double kOmegaZFilterPeriod = 8.0;
const double kSpeedFilterPeriod = 60.0;

}  // namespace

extern int debug;

bool ValidIMUGPS(const ControllerInput& in) {
  return !isnan(in.imu.position.longitude_deg) && !isnan(in.imu.position.latitude_deg) &&
         !(in.imu.position.longitude_deg == 0 && in.imu.position.latitude_deg == 0);
}

bool ValidGPS(const ControllerInput& in) {
  return !isnan(in.gps.longitude_deg) && !isnan(in.gps.latitude_deg) &&
         !(in.gps.longitude_deg == 0 && in.gps.latitude_deg == 0);
}


const int FilterBlock::len_0_6s = 0.6   / kSamplingPeriod + 0.5;
const int FilterBlock::len_1s   = 1.0   / kSamplingPeriod + 0.5;
const int FilterBlock::len_4s   = 4.0   / kSamplingPeriod + 0.5;
const int FilterBlock::len_30s  = 30.0  / kSamplingPeriod + 0.5;
const int FilterBlock::len_100s = 100.0 / kSamplingPeriod + 0.5;  // 100s, N.B. The ship control state Initial cannot be
                                                                  // shorter than this time period.

FilterBlock::FilterBlock()
  : valid_app_wind_(false),
    imu_fault_(false),
    imu_gps_fault_(false),
    gps_fault_(false),

    om_z_filter_(kOmegaZFilterPeriod / kSamplingPeriod),
    speed_filter_(kSpeedFilterPeriod / kSamplingPeriod),

    mag_app_filter_(len_4s),
    mag_true_filter_(len_100s),
    mag_aoa_filter_(len_30s),

    phi_z_filter_(),
    phi_z_wrap_(&phi_z_filter_),
    angle_app_filter_(len_4s),
    angle_app_wrap_(&angle_app_filter_),
    alpha_true_filter_x_(len_100s),
    alpha_true_filter_y_(len_100s),
    alpha_true_polar_(&alpha_true_filter_x_, &alpha_true_filter_y_),
    angle_aoa_filter_x_(len_30s),
    angle_aoa_filter_y_(len_30s),
    angle_aoa_polar_(&angle_aoa_filter_x_, &angle_aoa_filter_y_),
    gamma_sail_filter_(len_0_6s),
    gamma_sail_wrap_(&gamma_sail_filter_),
    gamma_sail_model_(0) {}


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
                         double gamma_sail_star_rad,
                         FilteredMeasurements* fil) {
  fil->Reset();
  // TODO: signal mixer (IMU + compass) comes here. It overwrites IMU data.
  imu_fault_ = isnan(in.imu.attitude.phi_z_rad) || isnan(in.imu.velocity.x_m_s);
  if (imu_fault_)
    fprintf(stderr, "IMU fault\n");
  imu_gps_fault_ = !ValidIMUGPS(in);
  if (imu_gps_fault_)
    fprintf(stderr, "IMU GPS fault\n");

  gps_fault_ = !ValidGPS(in);
  if (gps_fault_)
    fprintf(stderr, "Secondary GPS fault\n");


  // yaw (== bearing) from 2 independant sources: IMU Kalman filter, raw IMU magnetic sensor and
  // independant compass.
  // The IMU magnetic sensor is variable by +- 8 degrees in standstill so the IMU weight
  // is reduced to 0.225 in total. Effectively the IMU serves as a hot back-up.
  // The raw magnetic sensor IMU output is a rather noisy and often incorrect.
  double consensus = 0;
  double compass_phi_z = CompassMixer::Mix(in.imu.attitude.phi_z_rad, imu_fault_ ? 0 : 0.15,
                                           in.imu.compass.phi_z_rad, in.imu.compass.valid ? 0.075 : 0,
                                           in.compass_sensor.phi_z_rad, 1,  // invalid if outdated TODO
                                           &consensus);
  if (consensus >= 0.5) {
    fil->phi_z_boat = phi_z_wrap_.Filter(compass_phi_z);
    if (debug) {
      fprintf(stderr, "filtered compass phi_z %lf deg\n", Rad2Deg(fil->phi_z_boat));
    }
  } else {
    fprintf(stderr, "No compass consensus, %lf\n", consensus);
  }

  // Position
  double imu_lat = 0;
  double imu_lon = 0;
  double gps_lat = 0;
  double gps_lon = 0;
  double gps_cog_rad = 0;
  double gps_speed_m_s = 0;

  // NaNs irreversibly poison filters, so we have to prevent that.
  // With the GPS data we are not that picky about temporary faults.
  // TODO: If GPS is missing for 10 minutes, then we should log this and
  // the Skipper should get an info that this is the last known position, not the current one.
  if (!imu_gps_fault_) {
    ASSIGN_NOT_NAN(imu_lat, in.imu.position.latitude_deg);  // GPS-Data
    ASSIGN_NOT_NAN(imu_lon, in.imu.position.longitude_deg);
  }
  if (!gps_fault_) {
    ASSIGN_NOT_NAN(gps_lat, in.gps.latitude_deg);
    ASSIGN_NOT_NAN(gps_lon, in.gps.longitude_deg);
    ASSIGN_NOT_NAN(gps_cog_rad, in.gps.cog_rad);
    ASSIGN_NOT_NAN(gps_speed_m_s, in.gps.speed_m_s);
  }

  // The position values are not overwritten if but sensors fail.
  // TODO Mark them as stale after 15 minutes.
  if (imu_lat != 0 || gps_lat != 0) {
    // Due to the small relative errors of latitude and longitude the consensus is
    // less expressive for the position.
    // fprintf(stderr,"lat mix %lf %lf \n", imu_lat, gps_lat);
    fil->latitude_deg = CompassMixer::Mix(imu_lat, imu_gps_fault_ ? 0 : 0.51,
                                          0, 0,
                                          gps_lat, gps_fault_ ? 0 : 1,
                                          &consensus, false);  // No range check, we work with degrees here.
  }
  if (imu_lon != 0 || gps_lon != 0) {
    // fprintf(stderr,"lon mix %lf %lf \n", imu_lon, gps_lon);
    fil->longitude_deg = CompassMixer::Mix(imu_lon, imu_gps_fault_ ? 0 : 0.51,
                                           0, 0,
                                           gps_lon, gps_fault_ ? 0 : 1,
                                           &consensus, false);
  }

  // Speed
  // The GPS has no orientation (bearing) information so all speeds are
  // speed magnitudes. But we may drift backwards.
  if (!gps_fault_ &&
      fabs(DeltaOldNewRad(gps_cog_rad, fil->phi_z_boat)) > M_PI / 4) {
    gps_cog_rad = NormalizeRad(gps_cog_rad - M_PI);
    gps_speed_m_s = -gps_speed_m_s;
    fprintf(stderr, "GPS neg. speed %lf\n", gps_speed_m_s);
  }


  // For the time beeing we take the average speed of IMU and GPS.
  double weight_imu = imu_fault_ ? 0 : 0.5;
  double weight_gps = gps_fault_ ? 0 : 0.5;
  if (weight_imu == 0 && weight_gps == 0) {
    // If everything else fails we optimistically assume that we are making some speed forward.
    fil->mag_boat = 1;
  } else {
    double sum = weight_imu * in.imu.velocity.x_m_s;
    sum += weight_gps * gps_speed_m_s;
    fil->mag_boat = CensorSpeed(speed_filter_.Filter(sum / (weight_imu + weight_gps)));
  }
  if (debug) {
      fprintf(stderr, "raw boat speed*0.8 %6.3lf filtered %6.3lf m/s, lat_lon %.7lf %.7lf phi_z %6.3lf \n",
              in.imu.velocity.x_m_s, fil->mag_boat,
              in.imu.position.latitude_deg, in.imu.position.longitude_deg,
              in.imu.attitude.phi_z_rad);
      fprintf(stderr, "gps data speed %6.3lf m/s, lat_lon %.7lf %.7lf cog %6.3lf\n",
              gps_speed_m_s,
              in.gps.latitude_deg, in.gps.longitude_deg,
              SymmetricRad(gps_cog_rad));
  }

  if (!imu_fault_) {
    ASSIGN_NOT_NAN(fil->phi_x_rad, in.imu.attitude.phi_x_rad);      // roll angle, heel;
    ASSIGN_NOT_NAN(fil->phi_y_rad, in.imu.attitude.phi_y_rad);      // pitch, nick angle;
  }

  double om_z = 0;
  ASSIGN_NOT_NAN(om_z, in.imu.gyro.omega_z_rad_s);                 // yaw rate, rotational speed around z axis
  fil->omega_boat = om_z_filter_.Filter(om_z_med_.Filter(om_z));
  if (debug && false) {
    fprintf(stderr, "raw om_z %lg filtered %lg\n", om_z, fil->omega_boat);
  }

  ASSIGN_NOT_NAN(fil->temperature_c, in.imu.temperature_c);           // in deg Celsius

  if (debug && !(counter_ % 20)) {
    fprintf(stderr, "filter wind sensor: %c homed: %c wind sensor: %lf %lf\n",
            in.wind_sensor.valid ? 'V' : 'I',
            in.drives.homed_sail ? 'V' : 'I',
            in.wind_sensor.alpha_deg, in.wind_sensor.mag_m_s);
  }

  double alpha_wind_rad = 0;
  double mag_wind_m_s = 0;
  // The wind sensor updates the wind once per second, so initially it may be invalid.
  if (in.wind_sensor.valid) {
    alpha_wind_rad = NormalizeRad(Deg2Rad(in.wind_sensor.alpha_deg));
    mag_wind_m_s = in.wind_sensor.mag_m_s;
    Polar aoa_out(0, 0);
    Polar aoa(alpha_wind_rad + kWindSensorOffsetRad, mag_wind_m_s);
    angle_aoa_polar_.Filter(aoa, &aoa_out);
    fil->angle_aoa = aoa_out.AngleRad();
    fil->mag_aoa = aoa_out.Mag();
    // For AOA optimization
    if (logging_aoa && !(counter_ % 20)) {
      fprintf(stderr, "aoa: %lg deg %lg m/s cspeed %lg m/s\n",
              Rad2Deg(fil->angle_aoa), fil->mag_aoa, fil->mag_boat);
    }
  } else {
    fprintf(stderr, "aoa, sensor invalid\n");
  }

  if (in.drives.homed_sail && in.wind_sensor.valid) {
    // Delay the sail angle similar to the wind angle sensor signal
    // such that both signal paths have the same average delay.
    // Take the reference value of the sail angle because the sail drive status
    // has big and unpredictable delays.
    LimitRateWrapRad(gamma_sail_star_rad, kOmegaMaxSail * kSamplingPeriod, &gamma_sail_model_);
    double gamma_sail = SymmetricRad(gamma_sail_wrap_.Filter(gamma_sail_model_));

    // The wind sensor is telling where the wind is coming *from*, but we work
    // with motion vectors pointing where the wind is going *to*, thatswhy we have M_PI here.
    // kWindSensorOffsetRad reflects the mounting inaccuracies of mast, mast top unit and sensor,
    // it should be small. Note that the windcat also has a flag to correct the sensor rotation (-120degree).
    double angle_app = SymmetricRad(alpha_wind_rad - M_PI + kWindSensorOffsetRad + gamma_sail);
    double mag_app = mag_wind_m_s;
    if (mag_app == 0)
      angle_app = 0;

    Polar alpha_true_out(0, 0);
    if (!imu_fault_) {
      Polar wind_true(0, 0);
      TruePolar(Polar(angle_app,  mag_app),
                Polar(fil->phi_z_boat, fil->mag_boat),
                fil->phi_z_boat,
                &wind_true);
      alpha_true_polar_.Filter(wind_true, &alpha_true_out);
      if (debug)
        fprintf(stderr, "trueInOut alpha mag: %lg, %lg, filtered: %lg, %lg\n",
                wind_true.AngleRad(), wind_true.Mag(),
                alpha_true_out.AngleRad(), alpha_true_out.Mag());
      // The validity of the true wind takes some time and is flagged separately with
      // valid_true_wind.
    } else {
      if (debug)
        fprintf(stderr, "Imu failed -> Approximate true wind.\n");
      Polar wind_true(angle_app + fil->phi_z_boat, mag_app);
      alpha_true_polar_.Filter(wind_true, &alpha_true_out);
      if (debug)
        fprintf(stderr, "Approximated trueInOut alpha mag: %lg, %lg, filtered: %lg, %lg\n",
                NormalizeRad(angle_app + fil->phi_z_boat), mag_app,
                alpha_true_out.AngleRad(), alpha_true_out.Mag());
    }
    fil->alpha_true = alpha_true_out.AngleRad();
    fil->mag_true = alpha_true_out.Mag();

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
  ++counter_;
}


bool FilterBlock::ValidAppWind() {
  return valid_app_wind_;
}

bool FilterBlock::ValidTrueWind() {
  return alpha_true_polar_.ValidOutput() && !imu_fault_ && valid_app_wind_;  // TODO remove !imu_fault condition
}

bool FilterBlock::ValidSpeed() {
  return speed_filter_.ValidOutput() && !imu_fault_;
}
