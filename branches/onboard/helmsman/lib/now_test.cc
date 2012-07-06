// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
//
// Unit test for common/normalize
#include "now.h"

#include <math.h>
#include "lib/testing/testing.h"

ATEST(CommonNowTest, BasicSecond) {
 int64_t t = now_s();
 fprintf(stderr, "%llds\n", t);
 EXPECT_LT(1334662479LL, t);  // 2012-04-17 after lunch
 EXPECT_GT(1429270440LL, t);  // 2015-04-17 after lunch
}

ATEST(CommonNowTest, BasicMillis) {
 int64_t t = now_ms();
 fprintf(stderr, "%lldms\n", t);
 EXPECT_LT(1334662479000LL, t);
 EXPECT_GT(1429270440000LL, t);
}

ATEST(CommonNowTest, BasicMicros) {
 int64_t t = now_micros();
 fprintf(stderr, "%lldus\n", t);
 EXPECT_LT(1334662479000000LL, t);
 EXPECT_GT(1429270440000000LL, t);
}


int main(int argc, char* argv[]) {
  testing::RunAllTests();
  return 0;
}
