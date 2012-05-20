// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

// Filters for all measurement values plus preprocessing of wind vectors
// The signals from IMU and wind sensor are filtered through different filters.
// Besides that a valid_app_wind flag in provided in the filtered_ output indicating that
// the apparent wind is valid.
// The true wind vector is very noisy and is stabilized with a slow (100s)
// sliding average filter and its own valid_true_wind flag.

#ifndef HELMSMAN_FILTER_BLOCK_H
#define HELMSMAN_FILTER_BLOCK_H

#include "helmsman/controller_io.h"
#include "helmsman/compass_mixer.h"

#include "lib/filter/median_filter.h"
#include "lib/filter/sliding_average_filter.h"
#include "lib/filter/quick_sliding_average_filter.h"
#include "lib/filter/wrap_around_filter.h"


bool ValidGPS(const ControllerInput& in);

class FilterBlock {
 public:
  FilterBlock();
  void Filter(const ControllerInput& in, FilteredMeasurements* fil);
  void MakeSkipperInput(const FilteredMeasurements& fil, SkipperInput* to_skipper);
  bool ValidAppWind();
  bool ValidTrueWind();
  bool ValidSpeed();

 private:
  double CensorSpeed(double raw_speed);  // corrects the speed empirically

  bool valid_app_wind_;  // The filtered output data for the apparent wind are invalid initially. This is a temporary state.
  bool imu_fault_;       // No speed, no orientation.
  bool gps_fault_;       // time and position are missing

  const int len_1s_;     // 1s
  const int len_30s_;    // 30s (heel)
  const int len_100s_;   // 100s
  // for omega_z
  Median5Filter om_z_med_;
  SlidingAverageFilter om_z_filter_;

  // for speed
  SlidingAverageFilter speed_filter_;

  SlidingAverageFilter mag_app_filter_;
  SlidingAverageFilter mag_true_filter_;
  SlidingAverageFilter mag_aoa_filter_;

  // The yaw (phi_z) and wind directions are angles that can
  // wrap around at 360 degrees and need a wrap around filter.
  Median5Filter phi_z_filter_;
  WrapAroundFilter phi_z_wrap_;  // phi_z wraps around from 360 to 0 degree.
  SlidingAverageFilter angle_app_filter_;
  WrapAroundFilter angle_app_wrap_;
  QuickSlidingAverageFilter alpha_true_filter_;
  WrapAroundFilter alpha_true_wrap_;
  QuickSlidingAverageFilter angle_aoa_filter_;
  WrapAroundFilter angle_aoa_wrap_;
  CompassMixer compass_mixer_;
};


#endif  // HELMSMAN_FILTER_BLOCK_H
