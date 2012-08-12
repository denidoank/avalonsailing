// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "helmsman/skipper_input.h"

#include <math.h>
#include <stdio.h>

#include "common/check.h"
#include "common/now.h"

SkipperInput::SkipperInput() {
  Reset();
}

SkipperInput::SkipperInput(int64_t a_timestamp_ms,
                           double a_latitude_deg, double a_longitude_deg,
                           double a_angle_true_deg, double a_mag_true_kn)
    : timestamp_ms(a_timestamp_ms),
      latitude_deg(a_latitude_deg), longitude_deg(a_longitude_deg),
      angle_true_deg(a_angle_true_deg), mag_true_kn(a_mag_true_kn) {}

SkipperInput::SkipperInput(const char* line) {
  Reset();
  FromString(line);
}

void SkipperInput::Reset() {
  timestamp_ms = 0;
  latitude_deg = NAN;
  longitude_deg = NAN;
  angle_true_deg = NAN;
  mag_true_kn = NAN;
}

bool SkipperInput::Valid() {
  return !isnan(latitude_deg) &&
         !isnan(longitude_deg) &&
         !isnan(angle_true_deg) &&
         !isnan(mag_true_kn);
}

int SkipperInput::FromString(const char* line) {
  int advance = 0;
  int64_t ltimestamp_ms;
  double llatitude_deg;
  double llongitude_deg;
  double langle_true_deg;
  double lmag_true_kn;
  int items = sscanf(line, "skipper_input: timestamp_ms:%lld "
                    "latitude_deg:%lf longitude_deg:%lf "
                    "angle_true_deg:%lf mag_true_kn:%lf\n%n",
                    &ltimestamp_ms,
                    &llatitude_deg, &llongitude_deg,
                    &langle_true_deg, &lmag_true_kn, &advance);
  if (items == 5) {
    timestamp_ms = ltimestamp_ms;
    latitude_deg = llatitude_deg;
    longitude_deg = llongitude_deg;
    angle_true_deg = langle_true_deg;
    mag_true_kn = lmag_true_kn;
  }
  return items ==5 ? advance : 0;
}


std::string SkipperInput::ToString() const {
  char line[1024];
  int s = snprintf(line, sizeof line,
      "skipper_input: timestamp_ms:%lld latitude_deg:%.6lf longitude_deg:%.6lf "
      "angle_true_deg:%.2lf mag_true_kn:%.2lf\n",
      timestamp_ms, latitude_deg, longitude_deg, angle_true_deg, mag_true_kn);
  return std::string(line, s);
}

bool SkipperInput::operator!=(const SkipperInput r) {
  return latitude_deg != r.latitude_deg  ||
         longitude_deg != r.longitude_deg  ||
         angle_true_deg != r.angle_true_deg  ||
         mag_true_kn != r.mag_true_kn;
}
