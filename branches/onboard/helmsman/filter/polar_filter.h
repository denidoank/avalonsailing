// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2012

// A filter for polar input signals like angles that wrap around at 2pi.
// It is the callers responsibilty to meet the sampling condition f_signal < 1/2T .
#ifndef LIB_FILTER_POLAR_FILTER_H
#define LIB_FILTER_POLAR_FILTER_H

#include "filter_interface.h"
#include "helmsman/lib/polar.h"

class PolarFilter {
 public:
  // PolarFilter does not take ownership of filter and does not delete it.
  PolarFilter(FilterInterface* filter_x,
              FilterInterface* filter_y);
  virtual ~PolarFilter();
  void Filter(const Polar& in, Polar* out);
  void SetOutput(const Polar& in0);
  bool ValidOutput();
 private:

  FilterInterface* filter_x_;
  FilterInterface* filter_y_;
};

#endif  // LIB_FILTER_POLAR_FILTER_H
