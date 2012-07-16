// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

// An alternative solution for the problem would be the transformation of the vector into the
// cartesian space.

#include "lib/filter/polar_filter.h"

#include <math.h>
#include <stdio.h>

#include "common/check.h"
#include "common/normalize.h"


PolarFilter:: PolarFilter(FilterInterface* filter_x,
                                    FilterInterface* filter_y)
    : filter_x_(filter_x),
      filter_y_(filter_y) {
}

PolarFilter::~PolarFilter() { }

void PolarFilter::Filter(const Polar& in,
                         Polar* out) {
   // printf(" alpha mag %lf %lf\n", in.AngleRad(), in.Mag());
   double x = in.Mag() * cos(in.AngleRad());
   double y = in.Mag() * sin(in.AngleRad());

   double xf = filter_x_->Filter(x);
   double yf = filter_y_->Filter(y);

   Polar polar_out(atan2(yf, xf), sqrt(xf * xf + yf * yf));
   *out = polar_out;
}

bool PolarFilter::ValidOutput() {
  return filter_x_->ValidOutput() && filter_y_->ValidOutput();
}

void PolarFilter::SetOutput(const Polar& in0) {
  filter_x_->SetOutput(in0.Mag() * cos(in0.AngleRad()));
  filter_y_->SetOutput(in0.Mag() * sin(in0.AngleRad()));
}

