// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "skipper/target_circle.h"
#include "lib/testing/testing.h"

// Return the bearing on a grand circle course precisely,
// in radians. This is the bearing at the starting point
// which is not constant and will change over time!
// Aviation Formulary V1.33, by Ed Williams
// http://williams.best.vwh.net/avform.htm
double PreciseBearingDeg(double lat1_deg, double lon1_deg,
                         double lat2_deg, double lon2_deg) {
  double lat1 = Deg2Rad(lat1_deg);
  double lon1 = Deg2Rad(-lon1_deg);  // The author of the formula used + for Western longitudes (!).
  double lat2 = Deg2Rad(lat2_deg);
  double lon2 = Deg2Rad(-lon2_deg);
  double b = 180.0 / M_PI *
      -fmod(atan2(sin(lon2 - lon1)*cos(lat2),
            cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(lon2 - lon1)),
            2 * M_PI);
  while (b < 0) b += 360;
  return b;
}

// Distance in degrees (as the target circle radius).
// http://williams.best.vwh.net/avform.htm
double PreciseDistanceDeg(double lat1_deg, double lon1_deg,
                          double lat2_deg, double lon2_deg) {
  double lat1 = Deg2Rad(lat1_deg);
  double lon1 = Deg2Rad(-lon1_deg);  // The author of the formula used + for Western longitudes (!).
  double lat2 = Deg2Rad(lat2_deg);
  double lon2 = Deg2Rad(-lon2_deg);

  double x = (sin((lat1 - lat2) / 2));
  double y = (sin((lon1-lon2)/2));
  double d_rad = 2 * asin(sqrt(x * x + cos(lat1)*cos(lat2) * y * y));
  return 180.0 / M_PI * d_rad;
}


TEST(TargetCircle, All) {
  // Near the equator everything is so simple.
  TargetCircle t(0, 0, 1);
  EXPECT_FLOAT_EQ(0, t.ToDeg(0, 0));
  EXPECT_FLOAT_EQ(0, t.ToDeg(-2, 0));
  //                            x, y = lat, lon = Northing, Easting
  EXPECT_FLOAT_EQ(   0, t.ToDeg(-1,  0));
  EXPECT_FLOAT_EQ(  45, t.ToDeg(-1, -1));
  EXPECT_FLOAT_EQ(  90, t.ToDeg( 0, -1));
  EXPECT_FLOAT_EQ( 135, t.ToDeg( 1, -1));
  EXPECT_FLOAT_EQ( 180, t.ToDeg( 1,  0));
  EXPECT_FLOAT_EQ( 225, t.ToDeg( 1,  1));
  EXPECT_FLOAT_EQ( 270, t.ToDeg( 0,  1));
  EXPECT_FLOAT_EQ( 315, t.ToDeg(-1,  1));

  EXPECT_FLOAT_EQ( 225, t.ToDeg( 10,  10));
  EXPECT_FLOAT_EQ(1.414213562373095, t.Distance(1, 1));  // sqrt(2)
  EXPECT_FLOAT_EQ(1.414177660952114, PreciseDistanceDeg(1, 1, 0, 0));
  EXPECT_FLOAT_EQ(14.14213562373095, t.Distance(10, 10));  // sqrt(2) * 10
  EXPECT_FLOAT_EQ(14.10604426056637, PreciseDistanceDeg(10, 10, 0, 0));
  EXPECT_EQ(false, t.In(1.01, 0));
  EXPECT_EQ(true,  t.In(0.99, 0));
  EXPECT_EQ(false, t.In(2.01, 0, 2));  // expansion of the radius by a factor
  EXPECT_EQ(true,  t.In(1.99, 0, 2));

  EXPECT_FLOAT_EQ(0, t.ToDeg(0, 0));
  EXPECT_FLOAT_EQ(0, t.ToDeg(-2, 0));

  // because t is at [0, 0]
  EXPECT_FLOAT_EQ(   0, PreciseBearingDeg(-1,  0, 0, 0));
  EXPECT_FLOAT_EQ(  45.00436354465515, PreciseBearingDeg(-1, -1, 0, 0));
  EXPECT_FLOAT_EQ(  90, PreciseBearingDeg( 0, -1, 0, 0));
  EXPECT_FLOAT_EQ( 134.9956364553449, PreciseBearingDeg( 1, -1, 0, 0));
  EXPECT_FLOAT_EQ( 180, PreciseBearingDeg( 1,  0, 0, 0));
  EXPECT_FLOAT_EQ( 225.0043635446551, PreciseBearingDeg( 1,  1, 0, 0));
  EXPECT_FLOAT_EQ( 270, PreciseBearingDeg( 0,  1, 0, 0));
  EXPECT_FLOAT_EQ( 314.9956364553449, PreciseBearingDeg(-1,  1, 0, 0));

  EXPECT_FLOAT_EQ( 225.4385485867423, PreciseBearingDeg( 10,  10, 0, 0));
  }


  TEST(TargetCircle, Real) {
  TargetCircle t(50, 50, 1);
  double lon_stretch = 1.0 / cos(Deg2Rad(50));
  EXPECT_FLOAT_EQ(45, t.ToDeg(49,                50 - 1 * lon_stretch));
  EXPECT_FLOAT_EQ(30, t.ToDeg(50 - 0.8660254037, 50 - 0.5 * lon_stretch));
  EXPECT_FLOAT_EQ(60, t.ToDeg(50 - 0.5,          50 - 0.8660254037 * lon_stretch));

  EXPECT_FLOAT_EQ(   0, t.ToDeg(50 + -1, 50 +  0 * lon_stretch));
  EXPECT_FLOAT_EQ(  45, t.ToDeg(50 + -1, 50 + -1 * lon_stretch));
  EXPECT_FLOAT_EQ(  90, t.ToDeg(50 +  0, 50 + -1 * lon_stretch));
  EXPECT_FLOAT_EQ( 135, t.ToDeg(50 +  1, 50 + -1 * lon_stretch));
  EXPECT_FLOAT_EQ( 180, t.ToDeg(50 +  1, 50 +  0 * lon_stretch));
  EXPECT_FLOAT_EQ( 225, t.ToDeg(50 +  1, 50 +  1 * lon_stretch));
  EXPECT_FLOAT_EQ( 270, t.ToDeg(50 +  0, 50 +  1 * lon_stretch));
  EXPECT_FLOAT_EQ( 315, t.ToDeg(50 + -1, 50 +  1 * lon_stretch));

  EXPECT_FLOAT_EQ(1,       t.Distance(50 + -1, 50 +  0 * lon_stretch));
  EXPECT_FLOAT_EQ(sqrt(2), t.Distance(50 + -1, 50 + -1 * lon_stretch));
  EXPECT_FLOAT_EQ(1,       t.Distance(50 +  0, 50 + -1 * lon_stretch));
  EXPECT_FLOAT_EQ(sqrt(2), t.Distance(50 +  1, 50 + -1 * lon_stretch));
  EXPECT_FLOAT_EQ(1,       t.Distance(50 +  1, 50 +  0 * lon_stretch));
  EXPECT_FLOAT_EQ(sqrt(2), t.Distance(50 +  1, 50 +  1 * lon_stretch));
  EXPECT_FLOAT_EQ(1,       t.Distance(50 +  0, 50 +  1 * lon_stretch));
  EXPECT_FLOAT_EQ(sqrt(2), t.Distance(50 + -1, 50 +  1 * lon_stretch));

  EXPECT_FLOAT_EQ(   0,                PreciseBearingDeg(50 + -1, 50 +  0 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ(  44.70590614487342, PreciseBearingDeg(50 + -1, 50 + -1 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ(  89.40410807736497, PreciseBearingDeg(50 +  0, 50 + -1 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ( 134.6982126826714,  PreciseBearingDeg(50 +  1, 50 + -1 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ( 180,                PreciseBearingDeg(50 +  1, 50 +  0 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ( 225.3017873173286,  PreciseBearingDeg(50 +  1, 50 +  1 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ( 270.595891922635,   PreciseBearingDeg(50 +  0, 50 +  1 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ( 315.2940938551266,  PreciseBearingDeg(50 + -1, 50 +  1 * lon_stretch, 50, 50));

  EXPECT_FLOAT_EQ(1,                 PreciseDistanceDeg(50 + -1, 50 +  0 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ(1.421499989243906, PreciseDistanceDeg(50 + -1, 50 + -1 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ(1,                 PreciseDistanceDeg(50 +  0, 50 + -1 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ(1.406791603383072, PreciseDistanceDeg(50 +  1, 50 + -1 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ(1,                 PreciseDistanceDeg(50 +  1, 50 +  0 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ(1.406791603383072, PreciseDistanceDeg(50 +  1, 50 +  1 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ(1,                 PreciseDistanceDeg(50 +  0, 50 +  1 * lon_stretch, 50, 50));
  EXPECT_FLOAT_EQ(1.421499989243906, PreciseDistanceDeg(50 + -1, 50 +  1 * lon_stretch, 50, 50));
  // So for target circles of about 100km radius, the bearing error is less than
  // 0.7 degree. For real long distance grand circle navigation we would need the precise formulae.
  // Maybe used them in the Octave code to define the target circles?

  EXPECT_FLOAT_EQ( 45, t.ToDeg( 40,  50 - 10 * lon_stretch));
  EXPECT_FLOAT_EQ(sqrt(2), t.Distance(49, 50 - 1 * lon_stretch));
  EXPECT_FLOAT_EQ(sqrt(2) * 10, t.Distance(40, 50 - 10 * lon_stretch));
  EXPECT_EQ(false, t.In(1.01, 0));
  EXPECT_EQ(false, t.In(0.99, 0));
  EXPECT_EQ(false, t.In(2.01, 0, 2));  // expansion of the radius by a factor
  EXPECT_EQ(false, t.In(1.99, 0, 2));
  EXPECT_EQ(true,  t.In(51.99, 50, 2));
  EXPECT_EQ(true,  t.In(49.4, 50.6));

  EXPECT_FLOAT_EQ( 0, t.ToDeg( 50, 50));
}


int main(int argc, char* argv[]) {
  TargetCircle_All();
  TargetCircle_Real();
  return 0;
}
