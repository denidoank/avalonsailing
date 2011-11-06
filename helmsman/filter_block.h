// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

// Filters for all measurement values plus preprocessing of wind vectors
// The signals from IMU and wind sensor are filtered through 2 filters.
// First a median-over-the-last-5-samples filter which suppresses up to 2
// outliers at a delay of 3 ticks (0.3s).
// Secondly a sliding average filter over 1s , which is supposed to suppress
// noise. Combined both filters have a delay time of about 0.8s which has to be
// taken into account by the consumers.
// Besides that a valid flag in provided in the filtered_ output indicating that 
// all filters contain valid data.
// The true wind vector is especially noisy and is stabilized with a slow (100s)
// sliding average filter. 

#ifndef HELMSMAN_FILTER_BLOCK_H
#define HELMSMAN_FILTER_BLOCK_H

#include "helmsman/controller_io.h"
#include "helmsman/boat.h"             // kWindSensorOffsetRad

#include "lib/filter/median_filter.h"
#include "lib/filter/sliding_average_filter.h"
#include "lib/filter/quick_sliding_average_filter.h"
#include "lib/filter/wrap_around_filter.h"


class FilterBlock {
 public:
  FilterBlock();
  void Filter(const ControllerInput& in, FilteredMeasurements* fil);
  void MakeSkipperInput(const FilteredMeasurements& fil, SkipperInput* to_skipper);
  bool ValidTrueWind();

 private:
  bool valid_;
  bool imu_fault_;
  const int len_short_;  // filter length (1s)
  const int len_middle_; // TODO heel filter
  const int len_long_;   // filter length (100s) for true wind
  SlidingAverageFilter av_short_;
  SlidingAverageFilter av_middle_;
  SlidingAverageFilter av_long_;
  SlidingAverageFilter av_long_aoa_;

  // The wind directions are angles that can
  // wrap around at 360 degrees and need a wrap around filter.
  SlidingAverageFilter average_short_;
  QuickSlidingAverageFilter average_long_;
  QuickSlidingAverageFilter average_long_aoa_;
  WrapAroundFilter wrap_short_av_;
  WrapAroundFilter wrap_long_av_;
  WrapAroundFilter wrap_long_av_aoa_;
};


#endif  // HELMSMAN_FILTER_BLOCK_H
