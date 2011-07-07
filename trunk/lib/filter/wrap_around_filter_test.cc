// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "wrap_around_filter.h"

#include "common/delta_angle.h"
#include "common/normalize.h"
#include "lib/testing/testing.h"
#include "lib/util/stopwatch.h"

#include "sliding_average_filter.h"
#include "wrap_around_filter.h"
#include "median_filter.h"
#include "low_pass_filter.h"

typedef long long int64;

int64 Now() {
  assert(sizeof(int64) == 8);
  return StopWatch::GetTimestampMicros();
}

TEST(WrapAroundFilter, SlidingValidDCGainSetOutput) {
  WrapAroundFilter f(new SlidingAverageFilter(5));
  f.SetOutput(2);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(2);
    EXPECT_FLOAT_EQ(2, out);
    EXPECT_TRUE(f.ValidOutput());
  }
}

TEST(WrapAroundFilter, SlidingValidDCGain) {
  WrapAroundFilter f(new SlidingAverageFilter(5));
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(2);
    if (i >= 4) {
      EXPECT_TRUE(f.ValidOutput());
      EXPECT_FLOAT_EQ(2, out);
    } else {
      EXPECT_FALSE(f.ValidOutput());
    }
  }
}

TEST(WrapAroundFilter, SlidingFiftyPercent) {
  WrapAroundFilter f(new SlidingAverageFilter(10));
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(1);
    if (i == 4)
      EXPECT_FLOAT_EQ(1 * 0.5, out);
  }
}

TEST(WrapAroundFilter, SlidingZigZag) {
  WrapAroundFilter f(new SlidingAverageFilter(10));
  f.SetOutput(2);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i % 3 - 1 + 2);
    EXPECT_LT(1.85, out);
    EXPECT_GT(2.15, out);
  }
}


TEST(WrapAroundFilter, Median5ValidDCGainSetOutput) {
  WrapAroundFilter f(new Median5Filter());
  f.SetOutput(2);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(2);
    EXPECT_FLOAT_EQ(2, out);
    EXPECT_TRUE(f.ValidOutput());
  }
}

TEST(WrapAroundFilter, Median5ValidDCGain) {
  WrapAroundFilter f(new Median5Filter());
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(2);
    if (i >= 4) {
      EXPECT_TRUE(f.ValidOutput());
      EXPECT_FLOAT_EQ(2, out);
    } else {
      EXPECT_FALSE(f.ValidOutput());
    }
  }
}

TEST(WrapAroundFilter, Median3) {
  WrapAroundFilter f(new Median3Filter());
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(1);
    if (i >= 2) {
      EXPECT_TRUE(f.ValidOutput());
      EXPECT_EQ(1, out);
    } else {
      EXPECT_FALSE(f.ValidOutput());
    }
  }
}

TEST(WrapAroundFilter, Median3ZigZag) {
  WrapAroundFilter f(new Median3Filter());
  f.SetOutput(2);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i % 3 - 1 + 2);
    EXPECT_EQ(2, out);
  }
}

TEST(WrapAroundFilter, LowPass1FilterZigZagValid) {
  WrapAroundFilter f(new LowPass1Filter(10));
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i % 3 - 1 + 2);
    if (i >= 9) {
      EXPECT_TRUE(f.ValidOutput());
    } else {
      EXPECT_FALSE(f.ValidOutput());
    }
  }
}


TEST(WrapAroundFilter, LowPass1FilterZigZag) {
  WrapAroundFilter f(new LowPass1Filter(10));
  f.SetOutput(2);
  for (int i = 0; i < 20; ++i) {
    double out = f.Filter(i % 3 - 1 + 2);
    EXPECT_LT(1.85, out);
    EXPECT_GT(2.15, out);
    EXPECT_TRUE(f.ValidOutput());
  }
}

TEST(WrapAroundFilter, AllFilterRamp) {
  WrapAroundFilter f1(new SlidingAverageFilter(10));
  WrapAroundFilter f2(new LowPass1Filter(10));
  WrapAroundFilter f3(new Median3Filter());
  WrapAroundFilter f4(new Median5Filter());
  double prev1;
  double prev2;
  double prev3;
  double prev4;
  double increment = 2 * M_PI / 5;
  for (int cnt = 0; cnt < 50; ++cnt) {
    double in = NormalizeRad(cnt * increment);
    double out1 = f1.Filter(in);
    double out2 = f2.Filter(in);
    double out3 = f3.Filter(in);
    double out4 = f4.Filter(in);
    printf("%6.4f %6.4f %6.4f %6.4f\n", out1, out2, out3, out4);
    if (cnt == 1) {
      EXPECT_FALSE(f1.ValidOutput());
      EXPECT_FALSE(f2.ValidOutput());
      EXPECT_FALSE(f3.ValidOutput());
      EXPECT_FALSE(f4.ValidOutput());
    }
    if (cnt > 10) {
      EXPECT_TRUE(f1.ValidOutput());
      EXPECT_TRUE(f2.ValidOutput());
      EXPECT_TRUE(f3.ValidOutput());
      EXPECT_TRUE(f4.ValidOutput());
    }

    EXPECT_IN_INTERVAL(0, out1, 2 * M_PI);
    EXPECT_IN_INTERVAL(0, out2, 2 * M_PI);
    EXPECT_IN_INTERVAL(0, out3, 2 * M_PI);
    EXPECT_IN_INTERVAL(0, out4, 2 * M_PI);
    // In steady state, the output follows the input with the same gradient.
    if (cnt > 200) {
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev1, out1));
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev2, out2));
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev3, out3));
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev4, out4));
    }
    prev1 = out1;
    prev2 = out2;
    prev3 = out3;
    prev4 = out4;
  }
}

TEST(WrapAroundFilter, AllFilterRamp2) {
  WrapAroundFilter f1(new SlidingAverageFilter(10));
  WrapAroundFilter f2(new LowPass1Filter(10));
  WrapAroundFilter f3(new Median3Filter());
  WrapAroundFilter f4(new Median5Filter());
  double prev1;
  double prev2;
  double prev3;
  double prev4;
  double increment = 1;
  for (int cnt = 0; cnt < 5000; ++cnt) {
    double in = NormalizeRad(cnt * increment);
    double out1 = f1.Filter(in);
    double out2 = f2.Filter(in);
    double out3 = f3.Filter(in);
    double out4 = f4.Filter(in);
    //printf("%6.4f %6.4f %6.4f %6.4f\n", out1, out2, out3, out4);

    EXPECT_IN_INTERVAL(0, out1, 2 * M_PI);
    EXPECT_IN_INTERVAL(0, out2, 2 * M_PI);
    EXPECT_IN_INTERVAL(0, out3, 2 * M_PI);
    EXPECT_IN_INTERVAL(0, out4, 2 * M_PI);

    if (cnt == 1)
      EXPECT_FALSE(f1.ValidOutput());
    if (cnt > 10)
      EXPECT_TRUE(f1.ValidOutput());
    // In steady state, the output follows the input with the same gradient.
    if (cnt > 200) {
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev1, out1));
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev2, out2));
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev3, out3));
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev4, out4));
    }
    prev1 = out1;
    prev2 = out2;
    prev3 = out3;
    prev4 = out4;
  }
}

// All these filters have constant runtime in the range of 320ns (Desktop).
TEST(WrapAroundFilter, AllFilterTiming) {
  WrapAroundFilter f1(new SlidingAverageFilter(10));
  WrapAroundFilter f2(new LowPass1Filter(10));
  WrapAroundFilter f3(new Median3Filter());
  WrapAroundFilter f4(new Median5Filter());
  WrapAroundFilter f5(new SlidingAverageFilter(100));
  double prev1;
  double prev2;
  double prev3;
  double prev4;
  double prev5;
  double increment = 1;
  int64 micros1 = 0;
  int64 micros2 = 0;
  int64 micros3 = 0;
  int64 micros4 = 0;
  int64 micros5 = 0;
  const size_t rounds = 5000;
  for (int cnt = 0; cnt < rounds; ++cnt) {
    double in = NormalizeRad(cnt * increment);
    int64 n0 = Now();
    double out1 = f1.Filter(in);
    int64 n1 = Now();
    double out2 = f2.Filter(in);
    int64 n2 = Now();
    double out3 = f3.Filter(in);
    int64 n3 = Now();
    double out4 = f4.Filter(in);
    int64 n4 = Now();
    double out5 = f5.Filter(in);
    int64 n5 = Now();
    micros1 += n1 - n0;
    micros2 += n2 - n1;
    micros3 += n3 - n2;
    micros4 += n4 - n3;
    micros5 += n5 - n4;
    //printf("%6.4f %6.4f %6.4f %6.4f %6.4f\n", out1, out2, out3, out4, out5);

    EXPECT_IN_INTERVAL(0, out1, 2 * M_PI);
    EXPECT_IN_INTERVAL(0, out2, 2 * M_PI);
    EXPECT_IN_INTERVAL(0, out3, 2 * M_PI);
    EXPECT_IN_INTERVAL(0, out4, 2 * M_PI);
    EXPECT_IN_INTERVAL(0, out5, 2 * M_PI);

    // In steady state, the output follows the input with the same gradient.
    if (cnt > 200) {
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev1, out1));
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev2, out2));
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev3, out3));
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev4, out4));
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev5, out5));
    }
    if (cnt < 99)
      EXPECT_FALSE(f5.ValidOutput());
    if (cnt >= 99)
      EXPECT_TRUE(f5.ValidOutput());
    prev1 = out1;
    prev2 = out2;
    prev3 = out3;
    prev4 = out4;
    prev5 = out5;
  }
  printf("\nRuntimes/microseconds\n=================\n");
  printf("SlidingAverageFilter:    %6.4f micros\n", micros1 / static_cast<double>(rounds));
  printf("LowPass1Filter:          %6.4f micros\n", micros2 / static_cast<double>(rounds));
  printf("Median3Filter:           %6.4f micros\n", micros3 / static_cast<double>(rounds));
  printf("Median5Filter:           %6.4f micros\n", micros4 / static_cast<double>(rounds));
  printf("SlidingAverageFilter100: %6.4f micros\n", micros5 / static_cast<double>(rounds));
  printf("\n");
}


int main(int argc, char* argv[]) {
  WrapAroundFilter_SlidingValidDCGainSetOutput();
  WrapAroundFilter_SlidingValidDCGain();
  WrapAroundFilter_SlidingFiftyPercent();
  WrapAroundFilter_SlidingZigZag();
  WrapAroundFilter_Median5ValidDCGainSetOutput();
  WrapAroundFilter_Median5ValidDCGain();
  WrapAroundFilter_Median3();
  WrapAroundFilter_Median3ZigZag();
  WrapAroundFilter_LowPass1FilterZigZagValid();
  WrapAroundFilter_LowPass1FilterZigZag();
  WrapAroundFilter_AllFilterRamp();
  WrapAroundFilter_AllFilterRamp2();
  WrapAroundFilter_AllFilterTiming();
  return 0;
}
