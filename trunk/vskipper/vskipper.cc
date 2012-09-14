// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include <utility>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <stdio.h>
#include <syslog.h>

#include "common/normalize.h"
#include "common/polar_diagram.h"
#include "vskipper.h"

using namespace std;

namespace skipper {
namespace {

static const double kSafeDistance = 200;  // meters

// Skipper will try to plot a course that is safe for at least kMaxTimeWindow
// seconds. If it is unable to do so, it will start decreasing time window. If
// it cannot find bearing that is safe even for kMinTimeWindow seconds, it will
// pick the least dangerous one. For this reason, kMinTimeWindow should larger
// or equal to the interval between skipper invocations.
static const double kMaxTimeWindow = 15*60;  // seconds
static const double kMinTimeWindow = 60;  // seconds

// When estimating danger of some bearing B we also look at danger at bearings
// that differ by up to this angle from B. This essentially characterizes how
// well Avalon can maintain bearing set by skipper.
static const double kCorridorWidth = 5.01;  // degrees

// Helper object to set float format to fixed point with given precision.
struct prec {
  explicit prec(int p_) : p(p_) {}
  int p;
};
std::ostream& operator<<(std::ostream& out, const prec& p) {
  return out << fixed << setprecision(p.p);
}

struct LocalAis {
  const std::string* id;

  // Their bearing and speed
  Bearing bearing;
  double speed_m_s;

  // Bearing and distance from us to them
  Bearing us_them;
  double distance_m;
};

std::ostream& operator<<(std::ostream& out, const LocalAis& ais) {
  return out  << "Ship '" << *ais.id
              << "' at bearing=" << prec(1) << ais.us_them.deg()
              << "º, distance=" << prec(1) << ais.distance_m << "m";
}

void ComputeLocalAis(const AisInfo& them,
                     const AvalonState& us,
                     LocalAis* out) {
  out->id = &them.id;
  out->bearing = them.bearing;
  out->speed_m_s = them.speed_m_s;

  // Extrapolate their current position (since AIS reports ship positions with
  // some delay we extrapolate all positions based on us.timestamp_ms.
  int64_t d_time_ms = us.timestamp_ms - them.timestamp_ms;
  double distance_m = them.speed_m_s / 1000.0 * d_time_ms;
  LatLon pos = SphericalMove(them.position, them.bearing, distance_m);
  SphericalShortestPath(us.position, pos, &out->us_them, &out->distance_m);
}

double ExpectedVelocity(Bearing wind_from,
                        double wind_speed_m_s,
                        Bearing avalon) {
  if (wind_speed_m_s < 1e-9) {
    // TODO(zis): why doesn't ReadPolarDiagram do this?
    return 0;
  }
  double speed_m_s;
  bool dead_tack;
  bool dead_jibe;
  ReadPolarDiagram(wind_from.deg() - avalon.deg(), wind_speed_m_s,
                   &dead_tack, &dead_jibe, &speed_m_s);
  return speed_m_s;
}

// Returns how dangerous it is to be at @distance_m from another ship.
double DistanceDanger(double distance_m) {
  if (distance_m > kSafeDistance) {
    return 0;
  } else {
    return 1.0 - distance_m / kSafeDistance;
  }
}

// Returns probability of wind strength changing to fraction @f of its current
// speed (can be greater or smaller than one).
double WindFractionP(double f) {
  return max(0.0, 1 - fabs(f-1));
}

struct CandidateBearing {
  Bearing bearing;
  double expected_velocity_m_s;
  // Difference from target bearing (degrees)
  double bearing_diff;

  double danger;
  double corridor_danger;

  explicit CandidateBearing(Bearing b)
      : bearing(b),
        expected_velocity_m_s(0),
        bearing_diff(0),
        danger(0),
        corridor_danger(0) {
  }
};

std::ostream& operator<<(std::ostream& out, const CandidateBearing& c) {
  return out << "bearing=" << prec(0) << c.bearing.deg()
             << ", expected_v=" << prec(3) << c.expected_velocity_m_s
             << ", target_diff=" << prec(3) << c.bearing_diff
             << ", danger=" << prec(2) << c.danger;
}

struct BetterBearing {
  bool operator()(const CandidateBearing& a, const CandidateBearing& b) const {
    if (a.corridor_danger != b.corridor_danger) {
      return a.corridor_danger < b.corridor_danger;
    }
    return a.bearing_diff < b.bearing_diff;
  }
};

void SkipperImpl(const AvalonState& now,
                 const std::vector<LocalAis>& ships,
                 double time_window_s,
                 int debug,
                 bool* safe,
                 Bearing* out) {
  std::vector<CandidateBearing> candidates;
  // TODO(zis): Perhaps use a larger step (i.e. fewer than 360 candidates).
  for (int i = 0; i < 360; ++i) {
    // Make sure our target is one of the candidates.
    candidates.push_back(CandidateBearing(
        Bearing::Radians(NormalizeRad(now.target.rad() + i * M_PI / 180.0)) ));
  }

  for (size_t i = 0; i < candidates.size(); ++i) {
    CandidateBearing& c = candidates[i];

    c.expected_velocity_m_s =
        ExpectedVelocity(now.wind_from, now.wind_speed_m_s, c.bearing);
    c.bearing_diff = fabs(SymmetricDeg(c.bearing.deg() - now.target.deg()));

    for (double wind_fraction = 0; wind_fraction < 2.01; wind_fraction += 0.2) {
      double danger = 0;
      double speed_m_s = wind_fraction * c.expected_velocity_m_s;
      for (size_t k = 0; k < ships.size(); ++k) {
        double dist = MinDistance(c.bearing, speed_m_s,
                                  ships[k].bearing, ships[k].speed_m_s,
                                  ships[k].us_them, ships[k].distance_m,
                                  time_window_s);
        danger = max(danger, WindFractionP(wind_fraction)*DistanceDanger(dist));
      }
      c.danger += danger;
    }
  }

  // Compute corridor_danger
  for (size_t i = 0; i < candidates.size(); ++i) {
    double danger = 0;
    // Yes, this can be done more efficiently :)
    for (size_t j = 0; j < candidates.size(); ++j) {
      double diff = candidates[i].bearing.deg() - candidates[j].bearing.deg();
      if (fabs(SymmetricDeg(diff)) < kCorridorWidth) {
        danger += candidates[j].danger;
      }
    }
    candidates[i].corridor_danger = danger;
  }

  CandidateBearing& best =
      *min_element(candidates.begin(), candidates.end(), BetterBearing());
  *out = best.bearing;
  *safe = best.corridor_danger < 1e-9;
}

}  // namespace

Bearing RunVSkipper(const AvalonState& now,
                    const std::vector<AisInfo>& ais_in,
                    int debug) {
  fprintf(stderr, "RunVSkipper: %d other ships around \n", int(ais_in.size()));
  fprintf(stderr, "in : %6.2lf\n", now.target.deg());
  fprintf(stderr, "in wind_from:  %6.2lf\n", now.wind_from.deg());
  fprintf(stderr, "in wind_speed: %6.2lf\n", now.wind_speed_m_s);
  fprintf(stderr, "in pos lat deg: %6.2lf\n", now.position.lat_deg());
  fprintf(stderr, "in pos lon deg: %6.2lf\n", now.position.lon_deg());

  std::vector<LocalAis> ships(ais_in.size());
  for (size_t i = 0; i < ais_in.size(); ++i) {
    ComputeLocalAis(ais_in[i], now, &ships[i]);
    if (debug) cerr << ships[i] << "\n";
  }

  Bearing out;
  for (double time_window_s = kMaxTimeWindow;
       time_window_s >= kMinTimeWindow;
       time_window_s /= 2) {
    bool safe;
    SkipperImpl(now, ships, time_window_s, debug, &safe, &out);
    if (safe) {
      fprintf(stderr, "RunVSkipper: %6.2lf deg safe for the next %6.2lfs.\n", out.deg(), time_window_s);
      return out;
    }
  }

  syslog(LOG_EMERG, "No safe bearing found for the next %lf seconds!", kMinTimeWindow);
  out = Bearing::Degrees(kVSkipperNoWay);
  return out;

}

}  // skipper
