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

static const int kChannels = 10;

class FilterBlock {
 public:
  FilterBlock();
  void Filter(const ControllerInput& in, FilteredMeasurements* fil);
  void MakeSkipperInput(const FilteredMeasurements& fil, SkipperInput* to_skipper);
  bool ValidTrueWind();

 private:
  bool initial_;
  const int len_short_;  // filter length (1s)
  const int len_long_;   // filter length (100s) for true wind
  Median5Filter median_[kChannels];
  Median5Filter median_mag_true_;
  SlidingAverageFilter average_0_;
  SlidingAverageFilter average_1_;
  SlidingAverageFilter average_2_;
  SlidingAverageFilter average_3_;
  SlidingAverageFilter average_4_;
  SlidingAverageFilter average_5_;
  SlidingAverageFilter average_6_;
  SlidingAverageFilter average_7_;
  SlidingAverageFilter average_8_;
  SlidingAverageFilter average_9_;
  SlidingAverageFilter average_mag_true_;

  // 2 channels (the heading phi_z and the wind direction) are angles that can
  // wrap around at 360 degrees and need a wrap around filter.
  // 1 slow filter channel for the true wind direction, needs a wrap around filter.
  Median5Filter median_wr_1_;
  Median5Filter median_wr_2_;
  Median5Filter median_wr_3_;
  
  WrapAroundFilter wrap_med_1_;
  WrapAroundFilter wrap_med_2_;
  WrapAroundFilter wrap_med_3_;

  QuickSlidingAverageFilter average_wr_1_;
  QuickSlidingAverageFilter average_wr_2_;
  QuickSlidingAverageFilter average_wr_3_;
  WrapAroundFilter wrap_1_;
  WrapAroundFilter wrap_2_;
  WrapAroundFilter wrap_3_;
};


#endif  // HELMSMAN_FILTER_BLOCK_H
