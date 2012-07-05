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
                           double a_longitude_deg, double a_latitude_deg,
                           double a_angle_true_deg, double a_mag_true_kn)
    : timestamp_ms(a_timestamp_ms),
      longitude_deg(a_longitude_deg), latitude_deg(a_latitude_deg),
      angle_true_deg(a_angle_true_deg), mag_true_kn(a_mag_true_kn) {}

SkipperInput::SkipperInput(const char* line) {
  Reset();
  FromString(line);
}

void SkipperInput::Reset() {
  timestamp_ms = 0;
  longitude_deg = NAN;
  latitude_deg = NAN;
  angle_true_deg = NAN;
  mag_true_kn = NAN;
}

int SkipperInput::FromString(const char* line) {
  int advance = 0;
  int items = sscanf(line, "skipper_input: timestamp_ms:%lld "
                    "longitude_deg:%lf latitude_deg:%lf "
                    "angle_true_deg:%lf mag_true_kn:%lf\n%n",
                    &timestamp_ms,
                    &longitude_deg, &latitude_deg,
                    &angle_true_deg, &mag_true_kn, &advance);
  if (items != 5)
    Reset();
  return advance;
}


std::string SkipperInput::ToString() const {
  char line[1024];
  int s = snprintf(line, sizeof line,
      "skipper_input: timestamp_ms:%lld longitude_deg:%.6lf latitude_deg:%.6lf "
      "angle_true_deg:%.2lf mag_true_kn:%.2lf\n",
      timestamp_ms, longitude_deg, latitude_deg, angle_true_deg, mag_true_kn);
  return std::string(line, s);
}

bool SkipperInput::operator!=(const SkipperInput r) {
  return longitude_deg != r.longitude_deg  ||
         latitude_deg != r.latitude_deg  ||
         angle_true_deg != r.angle_true_deg  ||
         mag_true_kn != r.mag_true_kn;
}
