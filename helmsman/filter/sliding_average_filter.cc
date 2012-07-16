// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "sliding_average_filter.h"

#include "helmsman/lib/check.h"

SlidingAverageFilter::SlidingAverageFilter(int samples)
    : window_size_(samples),
      index_(0),
      valid_(false),
      bn_(1.0 / window_size_),
      sum_(0) {
  CHECK_GT(samples, 1);
  z_ = new double[samples];
  for (int i = 0; i < samples; ++i)
    z_[i] = 0;
}

// Wipe history as if all past input values had been equal to y0.
void SlidingAverageFilter::SetOutput(double y0) {
  for (int i = 0; i < window_size_; ++i)
    z_[i] = y0;
  sum_ = window_size_* y0;
  valid_ = true;
}

void SlidingAverageFilter::Shift(double shift) {
  for (int i = 0; i < window_size_; ++i)
    z_[i] += shift;
  sum_ += window_size_* shift;
}

void SlidingAverageFilter::NextIndex() {
  index_ = (index_ + 1) % window_size_;
  if (!index_)
    valid_ = true;
}

double SlidingAverageFilter::Filter(double in) {
  sum_ += in - z_[index_];
  z_[index_] = in;
  NextIndex();
  return bn_ * sum_;
}

bool SlidingAverageFilter::ValidOutput() {
  return valid_;
}

SlidingAverageFilter::~SlidingAverageFilter() {
  delete[] z_;
}
