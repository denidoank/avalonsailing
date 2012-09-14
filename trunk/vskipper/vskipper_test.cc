#include <algorithm>
#include "util.h"

#include "vskipper.h"
#include "lib/testing/testing.h"
#include "common/polar_diagram.h"

using namespace std;

#define EXPECT_NEAR(expected, error, act) { \
  double tmp_actual = act; \
  EXPECT_IN_INTERVAL(expected - error, tmp_actual, expected + error); \
}

namespace skipper {
AvalonState DefaultState() {
  AvalonState state;
  state.position = LatLon::Degrees(42, -15);
  state.target = Bearing::West();
  state.wind_from = Bearing::Degrees(180);
  state.wind_speed_m_s = 10;
  return state;
}

AisInfo MakeShip(const AvalonState& avalon,
                 double bearing, double distance,
                 double ship_bearing, double speed) {
  AisInfo res;
  res.timestamp_ms = avalon.timestamp_ms;
  res.position =
      SphericalMove(avalon.position, Bearing::Degrees(bearing), distance);
  res.bearing = Bearing::Degrees(ship_bearing);
  res.speed_m_s = speed;
  return res;
}

double ExpectedVelocity(Bearing wind_from, double wind_speed_m_s, Bearing avalon) {
  if (wind_speed_m_s < 1e-9) {
    // TODO(zis): why ReadPolarDiagram doesn't do it?
    return 0;
  }
  double speed_m_s;
  bool dead_tack;
  bool dead_jibe;
  ReadPolarDiagram(wind_from.deg() - avalon.deg(), wind_speed_m_s,
                   &dead_tack, &dead_jibe, &speed_m_s);
  return speed_m_s;
}

double Simulate(const AvalonState& avalon_in,
                const vector<AisInfo>& ships_in,
                int ticks, double tick_s) {
  AvalonState avalon = avalon_in;
  vector<AisInfo> ships = ships_in;
  double res = 1e11;

  for (int tick = 0; tick < ticks; ++tick) {
    Bearing bearing = RunVSkipper(avalon, ships, 0);
    double v = ExpectedVelocity(avalon.wind_from,
                                avalon.wind_speed_m_s,
                                bearing);
    double min_dist = 1e11;
    for (size_t i = 0; i < ships.size(); ++i) {
      Bearing a_b;
      double dist_a_b;
      SphericalShortestPath(
          avalon.position, ships[i].position, &a_b, &dist_a_b);
      double dist = MinDistance(bearing, v,
                                ships[i].bearing, ships[i].speed_m_s,
                                a_b, dist_a_b, tick_s);
      min_dist = min(min_dist, dist);

      ships[i].position = SphericalMove(ships[i].position,
                                        ships[i].bearing,
                                        ships[i].speed_m_s * tick_s);
    }

    res = min(res, min_dist);
    avalon.position = SphericalMove(avalon.position, bearing, v * tick_s);
  }

  return res;
}

ATEST(VSkipper, Smoke) {
  AvalonState state = DefaultState();
  Bearing actual = RunVSkipper(state, std::vector<AisInfo>(), 0);

  EXPECT_NEAR(270, 5, actual.deg());

  state.target = Bearing::Radians(0.003);
  actual = RunVSkipper(state, std::vector<AisInfo>(), 0);

  EXPECT_EQ(0.003, actual.rad());
}

ATEST(VSkipper, Around) {
  AvalonState state = DefaultState();
  for (int target = -180; target < 180; ++target) {
    state.target = Bearing::Degrees(target);
    Bearing actual = RunVSkipper(state, std::vector<AisInfo>(), 0);
    double diff = fabs(SymmetricDeg(target - actual.deg()));
    EXPECT_NEAR(0, 0.1, diff);
  }
}

ATEST(VSkipper, TreeInTheWay) {
  AvalonState state = DefaultState();
  std::vector<AisInfo> ships;
  ships.push_back(MakeShip(state, 270, 400, 0, 0));  // Stationary ship in front

  Bearing actual = RunVSkipper(state, ships, 0);
  // Safe tangent is at 30º. Plus another 5 for safety corridor.
  // EXPECT_NEAR(270 - 30 - 5, 1, actual.deg());  pre revision 600
  // Passing on the other side is good as well.
  EXPECT_NEAR(270 + 30 + 5, 1, actual.deg());
  // Go around it at close to min safe distance (slightly above due to safety
  // corridor).
  EXPECT_IN_INTERVAL(199, Simulate(state, ships, 10, 60), 300);
}

ATEST(VSkipper, TreeBehind) {
  AvalonState state = DefaultState();
  std::vector<AisInfo> ships;
  ships.push_back(MakeShip(state, 90, 400, 0, 0));  // Stationary ship behind

  Bearing actual = RunVSkipper(state, ships, 0);
  EXPECT_NEAR(270, 5, actual.deg());
  EXPECT_NEAR(400, 1, Simulate(state, ships, 10, 60));
}

ATEST(VSkipper, FastShipsInFront) {
  AvalonState state = DefaultState();
  std::vector<AisInfo> ships;
  ships.push_back(MakeShip(state, 270, 400, 0, 30));
  ships.push_back(MakeShip(state, 270, 400, 180, 30));

  Bearing actual = RunVSkipper(state, ships, 0);
  EXPECT_IN_INTERVAL(265, actual.deg(), 270);
}

ATEST(VSkipper, Pincer) {
  AvalonState state = DefaultState();
  std::vector<AisInfo> ships;
  ships.push_back(MakeShip(state, 42+180+30, 200, 42, 10));
  ships.push_back(MakeShip(state, 42+180-30, 200, 42, 10));

  EXPECT_NEAR(100, 1, Simulate(state, ships, 10, 60));
  Bearing actual = RunVSkipper(state, ships, 0);
  EXPECT_NEAR(42 + 180, 0, actual.deg());

  ships[0].speed_m_s = 1;
  ships[1].speed_m_s = 1;
  EXPECT_NEAR(200, 1, Simulate(state, ships, 10, 60));
}

/*
ATEST(VSkipper, Convoy) {
  AvalonState state = DefaultState();
  std::vector<AisInfo> ships;
  for (int i = 0; i < 12; ++i) {
    // Complete surround quickly moving towards 42º
    ships.push_back(MakeShip(state, i*30, 400, 42, 7));
  }

  // The safest choice is to move along staying within safe distance.
  EXPECT_LE(200, Simulate(state, ships, 30, 60));

  // Now ships are stationary
  for (int i = 0; i < ships.size(); ++i) {
    ships[i].speed_m_s = 0;
  }
  // This could be considered a bug. Since the surround has only 400m radius,
  // Avalon runs into it whichever direction it takes (since kMinTimeWindow is
  // 60 seconds, corresponds to how often skipper expects to be called, i.e. for
  // how far ahead it should plan a safe course). In this situation the best
  // choice is to squeeze between two neighbouring ships. Since every pair is
  // 200m apart, we shouldn't have to come closer than 100m.
  EXPECT_NEAR(100, 1, Simulate(state, ships, 10, 60));
  EXPECT_NEAR(42, 1, RunVSkipper(state, ships, 0).deg());

  // Now ships are significantly faster than Avalon.
  for (int i = 0; i < ships.size(); ++i) {
    ships[i].speed_m_s = 10;
  }
  // Can't follow them, have to squeeze inbetween.
  EXPECT_NEAR(42, 1, RunVSkipper(state, ships, 0).deg());
  EXPECT_NEAR(100, 1, Simulate(state, ships, 10, 60));
}
*/
}  // skipper

int main(int argc, char **argv) {
  return testing::RunAllTests();
}
