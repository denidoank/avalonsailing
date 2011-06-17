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

  // false indicates unreliable output due to initial condition.
  // All filters have to switch to true after their Filter method
  // beeing called a finite number of times.
  virtual bool ValidOutput() = 0;

  // For a quick startup set the output to the initially expected value.
  // By default the initial output value is zero.
  virtual void SetOutput(double y0) = 0;

  // Support filters for values wrapping around.
  virtual void Shift(double shift) = 0;

  virtual ~FilterInterface();
};

#endif  // LIB_FILTER_FILTER_INTERFACE_H
