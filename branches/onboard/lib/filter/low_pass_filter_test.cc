// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "lib/filter/low_pass_filter.h"

#include "lib/testing/testing.h"

TEST(LowPass1Filter, ValidDCGain) {
  LowPass1Filter f = LowPass1Filter(5);
  f.SetOutput(6);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(6);
    // printf(" %9d,  %9g, %s\n", 6, out, f.ValidOutput() ? "valid" : "filling-up");
    EXPECT_FLOAT_EQ(6, out);
    EXPECT_TRUE(f.ValidOutput());
  }
}

TEST(LowPass1Filter, Percent63) {
  LowPass1Filter f(10);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(10);
    // printf(" %9d,  %9g, %s\n", 10, out, f.ValidOutput() ? "valid" : "filling-up");
    if (i == 9) {
      EXPECT_IN_INTERVAL(10 * 0.61, out, 10 * 0.66);  // should be 0.63 (1/e)
    }
    if (i >= 9)
      EXPECT_TRUE(f.ValidOutput());
    else
      EXPECT_FALSE(f.ValidOutput());
  }
}

TEST(LowPass1Filter, ZigZag) {
  LowPass1Filter f(10);
  f.SetOutput(5);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i % 3 - 1 + 5);
    // printf(" %9d,  %9g, %s\n", i % 3 - 1 + 5, out, f.ValidOutput() ? "valid" : "filling-up");
    EXPECT_LT(4.85, out);
    EXPECT_GT(5.15, out);
  }
}

int main(int argc, char* argv[]) {
  LowPass1Filter_ValidDCGain();
  LowPass1Filter_Percent63();
  LowPass1Filter_ZigZag();
  return 0;
}
