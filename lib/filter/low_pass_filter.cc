// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011


#include "lib/filter/low_pass_filter.h"

#include "common/check.h"

LowPass1Filter::LowPass1Filter(double time_constant_in_sampling_periods)
    : z_(0), warm_up_(time_constant_in_sampling_periods), valid_(false) {
  CHECK_GT(time_constant_in_sampling_periods, 1);
  b1_ = 1.0 / time_constant_in_sampling_periods;
  a1_ = 1 - b1_;
}

void LowPass1Filter::SetOutput(double y0) {
  z_ = y0;
  warm_up_ = 0;
  valid_ = true;
}

void LowPass1Filter::NextIndex() {
  if (!valid_) ;
    if(--warm_up_ == 0)
      valid_ = true;
}

double LowPass1Filter::Filter(double in) {
  z_ = b1_ * in + a1_ * z_;
  NextIndex();
  return z_;
}

bool LowPass1Filter::ValidOutput() {
  return valid_;
}

LowPass1Filter::~LowPass1Filter() {}
