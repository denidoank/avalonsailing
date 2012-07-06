// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#include "median_filter.h"

#include "lib/testing/testing.h"

TEST(Median3, Ramp) {
  Median3Filter f;
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i);
    if (f.ValidOutput()) {
      EXPECT_FLOAT_EQ(i - 1, out);
      EXPECT_LE(2, i);
    }
  }
}

TEST(Median3, ZigZag) {
  Median3Filter f;
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i % 3 - 1);
    if (f.ValidOutput())
      EXPECT_FLOAT_EQ(0, out);
  }
}

TEST(Median3, OneSpike) {
  Median3Filter f;
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i % 5 < 1 ? 2 : 1);
    if (f.ValidOutput())
      EXPECT_FLOAT_EQ(1, out);
  }
}


TEST(Median5, Ramp) {
  Median5Filter f;
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i);
    if (f.ValidOutput()) {
      EXPECT_FLOAT_EQ(i - 2, out);
      EXPECT_LE(4, i);
    }
  }
}

TEST(Median5, ZigZag) {
  Median5Filter f;
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i % 3 - 1);
    if (f.ValidOutput())
      EXPECT_FLOAT_EQ(0, out);
  }
}

TEST(Median5, OneSpike) {
  Median5Filter f;
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i % 5 < 1 ? 2 : 1);
    if (f.ValidOutput())
      EXPECT_FLOAT_EQ(1, out);
  }
}

TEST(Median5, TwoSpikes) {
  Median5Filter f;
  for (int i = 0; i < 20; ++i) {
    int in = i % 5 < 2 ? 2 : 1;
    double out = f.Filter(in);
    // printf(" %9d,  %9g, %s\n", in, out,
    //        f.ValidOutput() ? "valid" : "filling-up");
    if (f.ValidOutput())
      EXPECT_FLOAT_EQ(1, out);
  }
}

int main(int argc, char* argv[]) {
  Median3_Ramp();
  Median3_ZigZag();
  Median3_OneSpike();
  Median5_Ramp();
  Median5_ZigZag();
  Median5_OneSpike();
  Median5_TwoSpikes();
  return 0;
}


