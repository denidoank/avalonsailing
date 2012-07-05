// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
// December 2011

#include <math.h>
#include "common/probe.h"
#include "common/check.h"

Probe::Probe() {
  Reset();
}

void Probe::Reset() {
  sum_ = 0;
  samples_ = 0;
}

void Probe::Measure(double in) {
  sum_ += in;
  ++samples_;
}

int Probe::Samples() {
  return samples_;
}

double Probe::Value() {
  if (samples_ == 0)
    return NAN;
  return sum_ / samples_;
}
