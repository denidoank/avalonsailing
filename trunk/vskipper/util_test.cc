// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Ivan Zauharodneu, Aug 2011

#include <algorithm>
#include "util.h"

#include "lib/testing/testing.h"

using std::min;
using std::max;

namespace skipper {

void Alternative(LatLon from, LatLon to, Bearing* b, double* d) {
  // NOTE: Very rough, assumes spherical Earth and can give "tenths of percent"
  // errors according to http://en.wikipedia.org/wiki/Great-circle_navigation
  double dlon = to.lon_rad() - from.lon_rad();
  double sa = cos(to.lat_rad())*sin(dlon);
  double sb = cos(from.lat_rad())*sin(to.lat_rad()) - sin(from.lat_rad())*cos(to.lat_rad())*cos(dlon);
  *b = Bearing::Radians(atan2(sa, sb));
  *d = 6371001 * atan2(
      sqrt(sa*sa+sb*sb),
      sin(from.lat_rad())*sin(to.lat_rad())+cos(from.lat_rad())*cos(to.lat_rad())*cos(dlon));
}

double GetBearing(void (*f)(const LatLon&, const LatLon&, Bearing*, double*),
                  double lat1, double lon1, double lat2, double lon2) {
  Bearing b;
  double tmp;
  f(LatLon::Degrees(lat1, lon1), LatLon::Degrees(lat2, lon2), &b, &tmp);
  return b.deg();
}

double GetDistance(void (*f)(const LatLon&, const LatLon&, Bearing*, double*),
                  double lat1, double lon1, double lat2, double lon2) {
  Bearing tmp;
  double d;
  f(LatLon::Degrees(lat1, lon1), LatLon::Degrees(lat2, lon2), &tmp, &d);
  return d;
}

#define EXPECT_NEAR(expected, error, act) { \
  double tmp_actual = act; \
  EXPECT_IN_INTERVAL(expected - error, tmp_actual, expected + error); \
}

ATEST(BearingDistance, SphericalBearing) {
  EXPECT_NEAR(  0, 1e-2, GetBearing(SphericalShortestPath, 0, 0,  1,  0));
  EXPECT_NEAR( 45, 1e-2, GetBearing(SphericalShortestPath, 0, 0,  1,  1));
  EXPECT_NEAR( 90, 1e-2, GetBearing(SphericalShortestPath, 0, 0,  0,  1));
  EXPECT_NEAR(135, 1e-2, GetBearing(SphericalShortestPath, 0, 0, -1,  1));
  EXPECT_NEAR(180, 1e-2, GetBearing(SphericalShortestPath, 0, 0, -1,  0));
  EXPECT_NEAR(225, 1e-2, GetBearing(SphericalShortestPath, 0, 0, -1, -1));
  EXPECT_NEAR(270, 1e-2, GetBearing(SphericalShortestPath, 0, 0,  0, -1));
  EXPECT_NEAR(315, 1e-2, GetBearing(SphericalShortestPath, 0, 0,  1, -1));
  // From Quai at China garden to Uetliberg, TV tower.
  EXPECT_NEAR(264.6, 1e-2, GetBearing(SphericalShortestPath, 47.3552, 8.5493,
                                                             47.3514, 8.4902));
}

ATEST(BearingDistance, SphericalDistance) {
  EXPECT_NEAR(111e3, 1e3, GetDistance(SphericalShortestPath, 0, 0,  1,  0));
  EXPECT_NEAR(157e3, 1e3, GetDistance(SphericalShortestPath, 0, 0,  1,  1));
  EXPECT_NEAR(111e3, 1e3, GetDistance(SphericalShortestPath, 0, 0,  0,  1));
  EXPECT_NEAR(157e3, 1e3, GetDistance(SphericalShortestPath, 0, 0, -1,  1));
  EXPECT_NEAR(111e3, 1e3, GetDistance(SphericalShortestPath, 0, 0, -1,  0));
  EXPECT_NEAR(157e3, 1e3, GetDistance(SphericalShortestPath, 0, 0, -1, -1));
  EXPECT_NEAR(111e3, 1e3, GetDistance(SphericalShortestPath, 0, 0,  0, -1));
  EXPECT_NEAR(157e3, 1e3, GetDistance(SphericalShortestPath, 0, 0,  1, -1));
  EXPECT_NEAR(0, 1e0, GetDistance(SphericalShortestPath, 0, 0,  0, 0));
}

// Take 1M random origin points on Earth, try to navigate to a random
// destination point within 1-2 degrees of the origin point.


ATEST(BearingDistance, SphericalRandom) {
  {
    /* crash for
    from
    LatLon: [-89.5467354 -63.9646163]
    to
    LatLon: [-90.0000000 -64.1450058]
    due to singularities ner the poles.
    */
    LatLon from = LatLon::Degrees(-79.5467354, -63.9646163);
    LatLon to = LatLon::Degrees(-80.0000000, -64.1450058);

    Bearing b;
    double dist;
    SphericalShortestPath(from, to, &b, &dist);

    // Check against alternative formula
    {
      Bearing alt_b;
      double alt_dist;
      Alternative(from, to, &alt_b, &alt_dist);

      fprintf(stderr, "distances / m: %lf %lf\n", dist, alt_dist);
      EXPECT_LT(fabs(SymmetricDeg(b.deg() - alt_b.deg())), 1e-4);
      EXPECT_LT(fabs(dist - alt_dist), 2);  //
    }

    LatLon actual = SphericalMove(from, b, dist);

    double error;
    SphericalShortestPath(to, actual, &b, &error);

    EXPECT_LT(error, 1.0);
  }

  // No tests nearer than 12 degrees off the poles.
  for (int i = 0; i < 1000000; ++i) {
    fprintf(stderr, "%d\n", i);
    double lat1 = rand() * 156.0 / RAND_MAX - 78.0;
    double lon1 = rand() * 360.0 / RAND_MAX - 180.0;
    LatLon from = LatLon::Degrees(lat1, lon1);
    double lat2 = min(78.0, max(-78.0, lat1 + rand() * 2.0 / RAND_MAX - 1.0));
    double lon2 = SymmetricDeg(lon1 + rand() * 2.0 / RAND_MAX - 1.0);
    LatLon to = LatLon::Degrees(lat2, lon2);

    Bearing b;
    double dist;
    SphericalShortestPath(from, to, &b, &dist);

    // Check against alternative formula
    {
      Bearing alt_b;
      double alt_dist;
      Alternative(from, to, &alt_b, &alt_dist);

      EXPECT_LT(fabs(SymmetricDeg(b.deg() - alt_b.deg())), 1e-4);
      fprintf(stderr, "distances / m: %lf %lf\n", dist, alt_dist);

      EXPECT_LT(fabs(dist - alt_dist), 1);
    }

    LatLon actual = SphericalMove(from, b, dist);

    double error;
    SphericalShortestPath(to, actual, &b, &error);

    EXPECT_LT(error, 1.0);
  }


}

ATEST(MinDistance, Still) {
  EXPECT_NEAR(250, 1e-9,
              MinDistance(Bearing::Degrees(27), 0,
                          Bearing::Degrees(183), 0,
                          Bearing::Degrees(75), 250,
                          5));
}

ATEST(MinDistance, SameSpot) {
  EXPECT_NEAR(0, 1e-9,
              MinDistance(Bearing::Degrees(27), 0,
                          Bearing::Degrees(183), 0,
                          Bearing::Degrees(75), 0,
                          5));
  EXPECT_NEAR(0, 1e-9,  // Doesn't matter whether ships are moving
              MinDistance(Bearing::Degrees(27), 10,
                          Bearing::Degrees(183), 20,
                          Bearing::Degrees(75), 0,
                          5));
}

ATEST(MinDistance, HeadOnCollision) {
  EXPECT_NEAR(0, 1e-9,
              MinDistance(Bearing::Degrees(32), 1,
                          Bearing::Degrees(32 + 180), 2,
                          Bearing::Degrees(32), 12,
                          5));
  EXPECT_NEAR(6, 1e-9,  // Not enough time
              MinDistance(Bearing::Degrees(32), 1,
                          Bearing::Degrees(32 + 180), 2,
                          Bearing::Degrees(32), 12,
                          2));
}

ATEST(MinDistance, CollisionIntoStationary) {
  EXPECT_NEAR(0, 1e-9,
              MinDistance(Bearing::Degrees(32), 1,
                          Bearing::Degrees(77), 0,
                          Bearing::Degrees(32), 2,
                          5));
  EXPECT_NEAR(2, 1e-9,  // Not enough time
              MinDistance(Bearing::Degrees(32), 1,
                          Bearing::Degrees(77), 0,
                          Bearing::Degrees(32), 7,
                          5));
}

ATEST(MinDistance, Parallel_SameDirection_AvalonInFront) {
  // Same speed => distance doesn't change
  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(192), 1,
                          Bearing::Degrees(192), 1,
                          Bearing::Degrees(72), 50,
                          5));
  // Avalon is faster => min_distance is initial distance
  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(192), 2,
                          Bearing::Degrees(192), 1,
                          Bearing::Degrees(72), 50,
                          5));
  // Avalon is slower => loses 5 meters (in projection)
  EXPECT_NEAR(sqrt(50*50 - 25*25 + 20*20), 1e-9,
              MinDistance(Bearing::Degrees(192), 1,
                          Bearing::Degrees(192), 2,
                          Bearing::Degrees(72), 50,
                          5));
}

ATEST(MinDistance, Parallel_SameDirection_AvalonBehind) {
  // Same speed => distance doesn't change
  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(192), 1,
                          Bearing::Degrees(192), 1,
                          Bearing::Degrees(132), 50,
                          5));
  // Avalon is faster => loses 5 meters
  EXPECT_NEAR(sqrt(50*50 - 25*25 + 20*20), 1e-9,
              MinDistance(Bearing::Degrees(192), 2,
                          Bearing::Degrees(192), 1,
                          Bearing::Degrees(132), 50,
                          5));
  // Avalon is slower => gains 5 meters => min_distance stays at 50
  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(192), 1,
                          Bearing::Degrees(192), 2,
                          Bearing::Degrees(132), 50,
                          5));
}

ATEST(MinDistance, Parallel_OppositeDirection_AvalonInFront) {
  // Velocities do not matter - since distance only increases, min_distance
  // stays at 50
  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(192), 1,
                          Bearing::Degrees(192 + 180), 1,
                          Bearing::Degrees(72), 50,
                          5));
  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(192), 2,
                          Bearing::Degrees(192 + 180), 1,
                          Bearing::Degrees(72), 50,
                          5));
  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(192), 1,
                          Bearing::Degrees(192 + 180), 2,
                          Bearing::Degrees(72), 50,
                          5));
}

ATEST(MinDistance, Parallel_OppositeDirection_AvalonBehind) {
  EXPECT_NEAR(sqrt(50*50 - 25*25 + 15*15), 1e-9,
              MinDistance(Bearing::Degrees(192), 1,
                          Bearing::Degrees(192 + 180), 1,
                          Bearing::Degrees(132), 50,
                          5));
  EXPECT_NEAR(sqrt(50*50 - 25*25), 1e-9,
              MinDistance(Bearing::Degrees(192), 2,
                          Bearing::Degrees(192 + 180), 8,
                          Bearing::Degrees(132), 50,
                          10));
}

ATEST(MinDistance, Collinear) {
  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(23), 1,
                          Bearing::Degrees(23), 1,
                          Bearing::Degrees(23), 50,
                          5));
  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(23), 1,
                          Bearing::Degrees(23), 2,
                          Bearing::Degrees(23), 50,
                          5));
  EXPECT_NEAR(45, 1e-9,
              MinDistance(Bearing::Degrees(23), 2,
                          Bearing::Degrees(23), 1,
                          Bearing::Degrees(23), 50,
                          5));

  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(23), 1,
                          Bearing::Degrees(23), 1,
                          Bearing::Degrees(23+180), 50,
                          5));
  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(23), 2,
                          Bearing::Degrees(23), 1,
                          Bearing::Degrees(23+180), 50,
                          5));
  EXPECT_NEAR(45, 1e-9,
              MinDistance(Bearing::Degrees(23), 1,
                          Bearing::Degrees(23), 2,
                          Bearing::Degrees(23+180), 50,
                          5));

  EXPECT_NEAR(50, 1e-9,
              MinDistance(Bearing::Degrees(23), 1,
                          Bearing::Degrees(23+180), 1,
                          Bearing::Degrees(23+180), 50,
                          5));
  EXPECT_NEAR(40, 1e-9,
              MinDistance(Bearing::Degrees(23), 1,
                          Bearing::Degrees(23+180), 1,
                          Bearing::Degrees(23), 50,
                          5));
  EXPECT_NEAR(0, 1e-9,
              MinDistance(Bearing::Degrees(23), 1,
                          Bearing::Degrees(23+180), 1,
                          Bearing::Degrees(23), 50,
                          30));
}
}  // skipper

int main(int argc, char **argv) {
  return testing::RunAllTests();
}
