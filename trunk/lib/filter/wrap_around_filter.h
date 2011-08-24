// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// A filter for radians angle input signals like angles that wrap around at 2pi.
#ifndef LIB_FILTER_WRAP_AROUND_FILTER_H
#define LIB_FILTER_WRAP_AROUND_FILTER_H

#include "lib/filter/filter_interface.h"

class WrapAroundFilter : public FilterInterface {
 public:
  WrapAroundFilter(FilterInterface* filter);
  virtual double Filter(double in_rad);
  virtual bool ValidOutput();
  virtual void SetOutput(double y0);

  virtual ~WrapAroundFilter();
 private:
  void Shift(double shift);

  FilterInterface* filter_;
  double period_;
  bool initial_;
  double prev_;
  double continuous_;
};

#endif  // LIB_FILTER_WRAP_AROUND_FILTER_H
