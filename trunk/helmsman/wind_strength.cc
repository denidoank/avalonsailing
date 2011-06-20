// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/wind_strength.h"

#include "common/check.h"
// All limits preliminary and subject of discussion
// and later optimization. (Histograms from some islands on the way?)
//
// * Below 1 knot (0.5m/s) true wind speed the direction of
//   true and apparent wind become unreliable, thus:
//   - the skipper cannot determine the dead angle for tacking correctly
//   - the sail controller cannot due proper alignment of the sail or is
//     permanently changing it
//   - the resulting boat speed through the water is so small that
//     the rudders don't produce enough force and the heading control
//     has difficulties
//   - the progress made does not justify high energy consumption
//
// * Above 18 knots of true wind speed the risk of
//   - overloading the sail and of
//   - getting the solar panels on deck or the boat hull damaged
//   is big.

namespace {
const double kCalmLimit_m_s = 0.5;
const double kCalmLimitHysteresis_m_s = 0.15;

const double kStormLimit_m_s = 10;
const double kStormLimitHysteresis_m_s = 2;
}  // namespace

WindStrengthRange WindStrength(WindStrengthRange previous_range,
                               double wind_m_s) {
  CHECK_GE(wind_m_s, 0.0);
  switch (previous_range) {
    case kCalmWind:
      if (wind_m_s > kCalmLimit_m_s + kCalmLimitHysteresis_m_s)
        return kNormalWind;
      return kCalmWind;
    case kNormalWind:
      if (wind_m_s < kCalmLimit_m_s - kCalmLimitHysteresis_m_s)
        return kCalmWind;
      if (wind_m_s > kStormLimit_m_s + kStormLimitHysteresis_m_s)
        return kStormWind;
      return kNormalWind;
    case kStormWind:
      if (wind_m_s < kStormLimit_m_s - kStormLimitHysteresis_m_s)
        return kNormalWind;
      return kStormWind;
    default:
      CHECK(false);
  }
  return kNormalWind;  // never reached
}
