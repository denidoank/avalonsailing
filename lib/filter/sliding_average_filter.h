// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// Sliding average filter, DC-gain 1.
#ifndef LIB_FILTER_SLIDING_AVERAGE_FILTER_H
#define LIB_FILTER_SLIDING_AVERAGE_FILTER_H

#include "lib/filter/filter_interface.h"

class SlidingAverageFilter : public FilterInterface {
 public:
  // Parametrize the filter with the number of samples to average over
  explicit SlidingAverageFilter(int samples);
  virtual double Filter(double in);
  virtual bool ValidOutput();
  virtual ~SlidingAverageFilter();
  // For a quick startup set the output to the initially expected value.
  // By default the initial output value is zero.
  virtual void SetOutput(double y0);
  // Support filters for values wrapping around.
  virtual void Shift(double shift);

 private:
  void NextIndex();

  int window_size_;
  double* z_;
  int index_;
  bool valid_;
  double bn_;
  double sum_;
};

#endif  // LIB_FILTER_SLIDING_AVERAGE_FILTER_H
