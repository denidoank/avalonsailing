// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/skipper_input.h"

#include "lib/testing/testing.h"

TEST(SkipperInput, All) {
  const char in[] = "skipper_input: timestamp_ms:4000000000 "
                    "longitude_deg:10 latitude_deg:11.10 "
                    "angle_true_deg:-33 mag_true_kn:4.\n";
  // The same with full precision (GPS: 6 digits).
  const char out[] = "skipper_input: timestamp_ms:4000000000 "
                     "longitude_deg:10.000000 latitude_deg:11.100000 "
                     "angle_true_deg:-33.00 mag_true_kn:4.00\n";

  const char in7[] = "skipper_input: timestamp_ms:7 "
                     "longitude_deg:7.000000 latitude_deg:7.000000 "
                     "angle_true_deg:7.00 mag_true_kn:7.00\n";
  const char out7[] = "skipper_input: timestamp_ms:7 "
                      "longitude_deg:7.000000 latitude_deg:7.000000 "
                      "angle_true_deg:7.00 mag_true_kn:7.00\n";

  SkipperInput x(in);

  x.timestamp_ms = 7;
  x.longitude_deg = x.latitude_deg = x.angle_true_deg = x.mag_true_kn = 7;
  EXPECT_EQ(out7, x.ToString());
  std::string resp = x.ToString();
  fprintf(stderr, "toString: >>%s<<\n",resp.c_str());

  SkipperInput s(out7);
  SkipperInput s7(in7);
  EXPECT_FALSE(s != s7);
  EXPECT_EQ(out7, s.ToString());

  s.FromString(in);
  EXPECT_EQ(out, s.ToString());
  EXPECT_EQ(4000000000LL, s.timestamp_ms);
  EXPECT_EQ(10, s.longitude_deg);
  EXPECT_EQ(11.1, s.latitude_deg);
  EXPECT_EQ(-33, s.angle_true_deg);
  EXPECT_EQ(4, s.mag_true_kn);

  SkipperInput t;
  EXPECT_EQ(0, t.timestamp_ms);
  EXPECT_TRUE(isnan(t.longitude_deg));
  EXPECT_TRUE(isnan(t.latitude_deg));
  EXPECT_TRUE(isnan(t.angle_true_deg));
  EXPECT_TRUE(isnan(t.mag_true_kn));
  t.FromString(in);
  EXPECT_EQ(out, t.ToString());
  EXPECT_FALSE(s != t);

  // mag_true missing
  t.FromString("skipper_input: timestamp_ms:7 longitude_deg:10 "
               "latitude_deg:11.10 angle_true_deg:-33 \n");
  EXPECT_EQ(0, t.timestamp_ms);
  EXPECT_TRUE(isnan(t.longitude_deg));
  EXPECT_TRUE(isnan(t.latitude_deg));
  EXPECT_TRUE(isnan(t.angle_true_deg));
  EXPECT_TRUE(isnan(t.mag_true_kn));

  s.Reset();
  EXPECT_EQ(0, t.timestamp_ms);
  EXPECT_TRUE(isnan(t.longitude_deg));
  EXPECT_TRUE(isnan(t.latitude_deg));
  EXPECT_TRUE(isnan(t.angle_true_deg));
  EXPECT_TRUE(isnan(t.mag_true_kn));
}

int main() {
  SkipperInput_All();
  return 0;
}
