// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#include "sliding_average_filter.h"

#include "lib/testing/testing.h"

TEST(SlidingAverageFilter, ValidDCGainSetOutput) {
  SlidingAverageFilter f(5);
  f.SetOutput(6);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(6);
    EXPECT_FLOAT_EQ(6, out);
    EXPECT_TRUE(f.ValidOutput());
  }
}

TEST(SlidingAverageFilter, ValidDCGain) {
  SlidingAverageFilter f(5);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(6);
    if (i >= 4) {
      EXPECT_TRUE(f.ValidOutput());
      EXPECT_FLOAT_EQ(6, out);
    } else {
      EXPECT_FALSE(f.ValidOutput());      
    }  
  }
}

TEST(SlidingAverageFilter, FiftyPercent) {
  SlidingAverageFilter f(10);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(4);
    if (i == 4)
      EXPECT_FLOAT_EQ(4 * 0.5, out);
  }
}
  
TEST(SlidingAverageFilter, ZigZag) {
  SlidingAverageFilter f(10);
  f.SetOutput(5);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i % 3 - 1 + 5);
    EXPECT_LT(4.85, out);
    EXPECT_GT(5.15, out);
  }
}

int main(int argc, char* argv[]) {
  SlidingAverageFilter_ValidDCGain();
  SlidingAverageFilter_FiftyPercent();
  SlidingAverageFilter_ZigZag();
  return 0;
}
