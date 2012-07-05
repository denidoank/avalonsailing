// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// Low pass filters (IIR).
// input coefficients shifted such that we get direct throuphput
// and minimal delay. (unlike a real low-pass), DC-gain 1.
#ifndef LIB_FILTER_LOW_PASS_FILTER_H
#define LIB_FILTER_LOW_PASS_FILTER_H

#include "lib/filter/filter_interface.h"

class LowPass1Filter : public FilterInterface {
 public:
  // Parametrize the filter with T_1 / T, i.e. if the filter constant is 5s and
  // the sampling period (=call frequency of this filter) is 0.1s then call
  // LowPass1Filter(50);
  explicit LowPass1Filter(double time_constant_in_sampling_periods);
  virtual ~LowPass1Filter();

  virtual double Filter(double in);
  virtual bool ValidOutput(); 
  // For a quick startup set the output to the initially expected value.
  // By default the initial output value is zero.
  virtual void SetOutput(double y0);
  // Support filters for values wrapping around.
  virtual void Shift(double shift);

 private:
  void NextIndex();
  
  double z_;
  int warm_up_;
  bool valid_;
  double a1_;
  double b1_;
};

#endif  // LIB_FILTER_LOW_PASS_FILTER_H
