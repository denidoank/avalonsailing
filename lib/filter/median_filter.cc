// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// 2 median filters over the last 3 or 5 input values.
#include "lib/filter/median_filter.h"

#include "lib/filter/filter_interface.h"
#include "lib/filter/median_n.h"


Median3Filter::Median3Filter() : index_(0), valid_(false) {
  for (int i = 0; i < 3; ++i)
    z_[i] = 0;
}

void Median3Filter::NextIndex() {
  index_ = (index_ + 1) % 3;
  if(!index_)
    valid_ = true;
}

double Median3Filter::Filter(double in) {
  z_[index_] = in;
  index_ = (index_ + 1) % 3;
  return Median3(z_[0], z_[1], z_[2]);
}

bool Median3Filter::ValidOutput() {
  return valid_;
}

Median3Filter::~Median3Filter() {}


Median5Filter::Median5Filter() : index_(0), valid_(false) {
  for (int i = 0; i < 5; ++i)
    z_[i] = 0;
}

void Median5Filter::NextIndex() {
  index_ = (index_ + 1) % 3;
  // If index_ has completed the cycle 0, 1, ... N-1, 0 then z_ is completely
  // filled with valid input data.
  if(!index_)
    valid_ = true;
}

double Median5Filter::Filter(double in) {
  z_[index_] = in;
  NextIndex();
  return Median5(z_[0], z_[1], z_[2], z_[3], z_[4]);
}

bool Median5Filter::ValidOutput() {
  return valid_;
}


Median5Filter::~Median5Filter() {}

