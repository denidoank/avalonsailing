// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "quick_sliding_average_filter.h"

#include <stdio.h>
#include "helmsman/lib/check.h"

QuickSlidingAverageFilter::QuickSlidingAverageFilter(int samples)
    : SlidingAverageFilter:: SlidingAverageFilter(samples),
      count_(samples / 2 + 1),
      scale_up_(samples / static_cast<double>(count_ - 1)) {
  CHECK_GT(samples, 1);
}

double QuickSlidingAverageFilter::Filter(double in) {
  if (count_ > 0)
    --count_;
  double y0 = SlidingAverageFilter::Filter(in);
  if (count_ == 1) {
    SlidingAverageFilter::SetOutput(y0 * scale_up_);
    return y0 * scale_up_;
  }
  return y0;
}

void QuickSlidingAverageFilter::SetOutput(double y0) {
  count_ = 0;
  SlidingAverageFilter::SetOutput(y0);
}

QuickSlidingAverageFilter::~QuickSlidingAverageFilter() {}

