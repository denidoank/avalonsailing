// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#include "lib/filter/quick_sliding_average_filter.h"

#include "lib/testing/testing.h"

TEST(QuickSlidingAverageFilter, ValidDCGainSetOutput) {
  QuickSlidingAverageFilter f(5);
  f.SetOutput(6);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(6);
    EXPECT_FLOAT_EQ(6, out);
    EXPECT_TRUE(f.ValidOutput());
  }
}

TEST(QuickSlidingAverageFilter, ValidDCGain) {
  QuickSlidingAverageFilter f(5);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(6);
    if (i >= 1) {                    // The SlidingAverageFilter has 4 here.
      EXPECT_TRUE(f.ValidOutput());
      EXPECT_FLOAT_EQ(6, out);
    } else {
      EXPECT_FALSE(f.ValidOutput());      
    }  
  }
}

TEST(QuickSlidingAverageFilter, FiftyPercentLength10) {
  QuickSlidingAverageFilter f(10);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(4);
    if (i == 4) {
      EXPECT_FLOAT_EQ(4, out);  // in contrast to 4 * 0.5 for the SlidingAverageFilter
      EXPECT_TRUE(f.ValidOutput());
    }
  }
}
  
TEST(QuickSlidingAverageFilter, StepResponse) {
  QuickSlidingAverageFilter f(10);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(4);
    if (i == 4) {
      EXPECT_FLOAT_EQ(4, out);  // in contrast to 4 * 0.5 for the SlidingAverageFilter
      EXPECT_TRUE(f.ValidOutput());
    }
  }
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(11);
    if (i == 10) {
      EXPECT_FLOAT_EQ(11, out);  // in contrast to 4 * 0.5 for the SlidingAverageFilter
      EXPECT_TRUE(f.ValidOutput());
    }
  }
}

TEST(QuickSlidingAverageFilter, FiftyPercentLength11) {
  QuickSlidingAverageFilter f(11);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(4);
    if (i == 4) {
      EXPECT_FLOAT_EQ(4, out);  // in contrast to 4 * 0.5 for the SlidingAverageFilter
      EXPECT_TRUE(f.ValidOutput());
    }
  }
}
  
TEST(QuickSlidingAverageFilter, ZigZag) {
  QuickSlidingAverageFilter f(10);
  f.SetOutput(5);
  for (int i = 0; i < 200; ++i) {
    double out = f.Filter(i % 3 - 1 + 5);
    EXPECT_LT(4.85, out);
    EXPECT_GT(5.15, out);
  }
}

int main(int argc, char* argv[]) {
  QuickSlidingAverageFilter_ValidDCGain();
  QuickSlidingAverageFilter_FiftyPercentLength10();
  QuickSlidingAverageFilter_FiftyPercentLength11();
  QuickSlidingAverageFilter_ZigZag();
  return 0;
}
