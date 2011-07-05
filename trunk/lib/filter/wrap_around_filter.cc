// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "lib/filter/wrap_around_filter.h"

#include <math.h>
#include <stdio.h>
#include "lib/fm/log.h"

#include "common/check.h"
#include "common/normalize.h"


WrapAroundFilter:: WrapAroundFilter(FilterInterface* filter)
    : filter_(filter),
      period_(2 * M_PI),
      initial_(true) {
  CHECK_GT(period_, 0);
}

// Returns filtered value in [0, 2*pi)
double WrapAroundFilter::Filter(double in) {
  CHECK_IN_INTERVAL(0, in, 2 * M_PI);
  if (initial_) {
    prev_ = in;
    initial_= false;
    continuous_ = in;
  }
  CHECK_LT(in, 10);  // expect radians here
  CHECK_GT(in, -10);
  double delta = in - prev_;
  prev_ = in;
  // Force result into [-pi, pi)
  if (delta >= period_ / 2) {
    delta -= period_;
  } else if (delta < -period_ / 2) {
    delta += period_;
  }
  //printf("delta:   %6.4f \n", delta);
  CHECK_GT(M_PI, delta);
  CHECK_LE(-M_PI, delta);
  // continuous_ doesn't jump.
  continuous_ += delta;

  // If the magnitude of continuous becomes too big then we
  // loose precision. So we jump back into the [0, 2pi) range
  // occasionally.
  if (continuous_ > 32 * M_PI) {
    Shift(-32 * M_PI);
  }
  if (continuous_ < -32 * M_PI) {
    Shift(32 * M_PI);
  }

  //printf("cont: %6.4f \n", continuous_);
  double out = filter_->Filter(continuous_);
  return(NormalizeRad(out));
}

bool WrapAroundFilter::ValidOutput() {
  return filter_->ValidOutput();
}

void WrapAroundFilter::SetOutput(double y0) {
  continuous_ = y0;
  filter_->SetOutput(y0);
}

void WrapAroundFilter::Shift(double shift) {
  filter_->Shift(shift);
  continuous_ += shift;
  // Here we might accumulate errors.
  // If there is a deviation between shift and period_,
  // the this deviation gets accumulated here.
  continuous_ = prev_;
}

WrapAroundFilter::~WrapAroundFilter() {}
