// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// 2 median filters over the last 3 or 5 input values.
#ifndef LIB_FILTER_MEDIAN_FILTER_H
#define LIB_FILTER_MEDIAN_FILTER_H

#include "filter_interface.h"

class Median3Filter : public FilterInterface {
 public:
  Median3Filter();
  virtual double Filter(double in);
  virtual bool ValidOutput();
  virtual ~Median3Filter();
  virtual void SetOutput(double y0);
  virtual void Shift(double shift);
 private:
  void NextIndex();

  static const int len = 3;
  double z_[3];
  int index_;
  bool valid_;
};


class Median5Filter : public FilterInterface {
 public:
  Median5Filter();
  virtual double Filter(double in);
  virtual bool ValidOutput();
  virtual void SetOutput(double y0);
  virtual void Shift(double shift);
  virtual ~Median5Filter();
 private:
  void NextIndex();

  double z_[5];
  int index_;
  bool valid_;
};

#endif  // LIB_FILTER_MEDIAN_FILTER_H
