// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011

#include "skipper/skipper_internal.h"

#include "common/unknown.h"
#include "common/convert.h"
#include "common/delta_angle.h"
#include "common/polar_diagram.h"
#include "helmsman/normal_controller.h"
#include "lib/fm/log.h"
#include "skipper/planner.h"

extern int debug;

static const double kDefaultDirection = 225;  // SouthWest as an approximation of the whole journey.

double SkipperInternal::old_alpha_star_deg_ = kDefaultDirection;
WindStrengthRange SkipperInternal::wind_strength_ = kCalmWind;

void SkipperInternal::Run(const SkipperInput& in,
                          const vector<skipper::AisInfo>& ais,
                          double* alpha_star_deg) {

  if (in.angle_true_deg == kUnknown ||
      in.mag_true_kn == kUnknown ||
      isnan(in.angle_true_deg) ||
      isnan(in.mag_true_kn)) {
    *alpha_star_deg = kDefaultDirection;  // Southwest is our general direction.
    fprintf(stderr, "No true wind info so far, going SW.\n");
    return;
  }

  wind_strength_ = WindStrength(wind_strength_, KnotsToMeterPerSecond(in.mag_true_kn));

  double planned = 0;
  double safe = 0;
  if (in.longitude_deg == kUnknown ||
      in.latitude_deg == kUnknown ||
      isnan(in.longitude_deg) ||
      isnan(in.latitude_deg)) {
    if (!Planner::Initialized()) {
      *alpha_star_deg = kDefaultDirection;  // Southwest is our general direction.
      fprintf(stderr, "No position info so far, going SW.\n");
      return;
    } else {
      // Temporary loss of GPS fix. This might happen due to big
      // waves washing over the GPS antenna, excessive boat heel or
      // other temporary effects. But we have had a GPS fix already and we know
      // were to go for the next days, so it is ok to go on.
      planned = old_alpha_star_deg_;
      // Without GPS fix there is no way to find a safe course, but it is likely
      // that our last safe course is still valid.
      safe = old_alpha_star_deg_;
    }
  } else {
    planned = Planner::ToDeg(in.latitude_deg, in.longitude_deg);
    // Finally the helmsman will steer a +-15 deg course relative to the wind (jibe zone).
    if (kStormWind == wind_strength_) {
      planned = in.angle_true_deg;
      fprintf(stderr, "Storm, going downwind to %6.2lf deg.\n", planned);
    }
    safe = RunCollisionAvoider(planned, in, ais);
    old_alpha_star_deg_ = safe;
  }

  if (fabs(planned - safe) > 0.1)
      fprintf(stderr,
              "Override %8.6lg with %8.6lg because it is not collision free.\n",
              planned, safe);

  double feasible = BestSailableHeadingDeg(safe, in.angle_true_deg);

  if (fabs(DeltaOldNewDeg(safe, feasible)) > 0.1)
    fprintf(stderr, "Override %8.6lg deg with %8.6lg deg because it is not sailable.\n",
            safe, feasible);

  if (debug)
    fprintf(stderr, "planner %lg , alpha* %lg,  true wind: %lg, feasible: %lg\n",
            planned, safe, in.angle_true_deg, feasible);

  *alpha_star_deg = feasible;
}

void SkipperInternal::Init(const SkipperInput& in) {
  Planner::Init(LatLon(in.latitude_deg, in.longitude_deg));
}

bool SkipperInternal::TargetReached(const LatLon& lat_lon){
  return Planner::TargetReached(lat_lon);
}

double SkipperInternal::RunCollisionAvoider(
    double planned,
    const SkipperInput& in,
    const vector<skipper::AisInfo>& ais) {
  skipper::AvalonState avalon;
  avalon.timestamp_ms = ais.size() ? ais[0].timestamp_ms : 0;
  avalon.position = skipper::LatLon(in.longitude_deg, in.latitude_deg);
  avalon.target = skipper::Bearing::Degrees(planned);
  avalon.wind_from = skipper::Bearing::Degrees(in.angle_true_deg + 180);
  avalon.wind_speed_m_s = KnotsToMeterPerSecond(in.mag_true_kn);
  skipper::Bearing skipper_out = RunVSkipper(avalon, ais, 0);
  return skipper_out.deg();
}



