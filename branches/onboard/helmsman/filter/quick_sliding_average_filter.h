// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// Sliding average filter, DC-gain 1, but returns Valid
// after half of the entries are filled already.
// So if samples are 100, Valid() goes true after 50 calls of Filter. 
#ifndef LIB_FILTER_QUICK_SLIDING_AVERAGE_FILTER_H
#define LIB_FILTER_QUICK_SLIDING_AVERAGE_FILTER_H

#include "lib/filter/sliding_average_filter.h"

class QuickSlidingAverageFilter : public SlidingAverageFilter {
 public:
  // Parametrize the filter with the number of samples to average over
  explicit QuickSlidingAverageFilter(int samples);
  virtual double Filter(double in);
  virtual void SetOutput(double y0);
  virtual ~ QuickSlidingAverageFilter();

 private:
  int count_;
  double scale_up_;
};

#endif  // LIB_FILTER_QUICK_SLIDING_AVERAGE_FILTER_H
