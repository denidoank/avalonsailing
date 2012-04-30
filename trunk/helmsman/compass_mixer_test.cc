// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "compass_mixer.h"



#include "common/array_size.h"
#include "common/delta_angle.h"
#include "common/normalize.h"
#include "lib/testing/testing.h"

#include "lib/filter/sliding_average_filter.h"
#include "lib/filter/low_pass_filter.h"
#include "lib/filter/median_filter.h"
#include "lib/filter/wrap_around_filter.h"

extern int debug;

ATEST(CompassMixer, Basic) {
  // WrapAroundFilter f1(new SlidingAverageFilter(6));
  // WrapAroundFilter f2(new LowPass1Filter(5));
  // WrapAroundFilter f3(new Median5Filter());
  // Three filter with different phase delay. mix is equal to second
  WrapAroundFilter f1(new SlidingAverageFilter(6));
  WrapAroundFilter f2(new SlidingAverageFilter(5));
  WrapAroundFilter f3(new SlidingAverageFilter(4));
  WrapAroundFilter* filters[3] = {&f1, &f2, &f3};
  debug = 0;
  double out[3] = {0, 0, 0};
  double prev;
  double increment = 1;
  prev = 0;
  CompassMixer mixer;
  double consensus;
  for (int cnt = 0; cnt < 5000; ++cnt) {
    double in = NormalizeRad(cnt * increment);
    for  (int n = 0; n < ARRAY_SIZE(filters); ++n)
      out[n] = filters[n]->Filter(in);

    double mix = mixer.Mix(out[0], 1, out[1], 1, out[2], 1, &consensus);
    // printf("%6.4lf ( %6.4lf %6.4lf %6.4lf) %6.4lf\n", in, out[0], out[1], out[2], mix);

    EXPECT_IN_INTERVAL(0, mix, 2 * M_PI);
    if (cnt > 5) {
      EXPECT_FLOAT_EQ(out[1], mix);
    }
    EXPECT_LT(0.5, consensus);
    // In steady state, the output follows the input with the same gradient.
    if (cnt > 10) {
      EXPECT_FLOAT_EQ(increment, DeltaOldNewRad(prev, mix));
    }
    prev = mix;
  }
}


ATEST(CompassMixer, NoConsensus) {
  CompassMixer mixer;
  double consensus;
  double mix = mixer.Mix(2.11, 1, 2.11, 1, 2.11, 1, &consensus);
  EXPECT_FLOAT_EQ(1, consensus);
  EXPECT_FLOAT_EQ(2.11, mix);
  // Garbage in, garbage out.
  EXPECT_EQ(0, mixer.Mix(0, 0,  0, 0,  0, 0, &consensus));
  EXPECT_EQ(0, consensus);

  EXPECT_FLOAT_EQ(3, mixer.Mix(3, 1,  0, 0,  0, 0, &consensus));
  EXPECT_FLOAT_EQ(1, consensus);

  EXPECT_FLOAT_EQ(3, mixer.Mix(3, 1,  3, 1,  0, 0, &consensus));
  EXPECT_FLOAT_EQ(1, consensus);

  EXPECT_FLOAT_EQ(3, mixer.Mix(3, 1,  0, 0,  3, 1, &consensus));
  EXPECT_FLOAT_EQ(1, consensus);

  EXPECT_FLOAT_EQ(3, mixer.Mix(3, 1,  3, 1,  3, 1, &consensus));
  EXPECT_FLOAT_EQ(1, consensus);
  // not enough weight
  EXPECT_FLOAT_EQ(3, mixer.Mix(3, 0.3,  0, 0,  0, 0, &consensus));
  EXPECT_FLOAT_EQ(0.3, consensus);

  EXPECT_FLOAT_EQ(3, mixer.Mix(3, 0.1,  3, 0.1,  3, 0.1, &consensus));
  EXPECT_FLOAT_EQ(0.3, consensus);

  // Divergence
  EXPECT_FLOAT_EQ(3, mixer.Mix(3 - M_PI/4, 1,  3 + M_PI/4, 1,  0, 0, &consensus));
  EXPECT_FLOAT_EQ(0.707107, consensus);

  EXPECT_FLOAT_EQ(3, mixer.Mix(3 - M_PI/3, 1,  3 + M_PI/3, 1,  0, 0, &consensus));
  EXPECT_FLOAT_EQ(0.5, consensus);

  EXPECT_FLOAT_EQ(3, mixer.Mix(3 - M_PI/3, 1,  3, 1,  3 + M_PI/3, 1,  &consensus));
  EXPECT_FLOAT_EQ(0.6666667, consensus);
}

int main(int argc, char* argv[]) {
  return testing::RunAllTests();
}
