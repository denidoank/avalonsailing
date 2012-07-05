// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef LIB_FILTER_FILTER_INTERFACE_H
#define LIB_FILTER_FILTER_INTERFACE_H

class FilterInterface {
 public:
  // On calling the Filter method with the current input the
  // current filter output is produced.
  virtual double Filter(double in) = 0;

  // false indicates unreliable output due to the fact that
  // the assumed initial condition (the history of value
  // contains zeros only) is not necessarily true.
  // All filters switch to true sooner or later if their
  // Filter method is repeatedly called.
  virtual bool ValidOutput() = 0;

  // For a quick startup set the history to y0.
  // (By default the initial output value is zero.)
  // Side effect: makes ValidOutput return true afterwards.
  virtual void SetOutput(double y0) = 0;

  // Support for filters for values wrapping around.
  virtual void Shift(double shift) = 0;

  virtual ~FilterInterface();
};

#endif  // LIB_FILTER_FILTER_INTERFACE_H
