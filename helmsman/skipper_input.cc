// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#include "helmsman/skipper_input.h"

#include <stdio.h>
#include <string.h>

#include "common/check.h"

SkipperInput::SkipperInput() {
  Reset();
}

void SkipperInput::Reset() {
  longitude_deg = 0;
  latitude_deg = 0;
  angle_true_deg = 0;
  mag_true_kn = 0;
}

SkipperInput::SkipperInput(const std::string& kvline) {
  Reset();
  const char* line = kvline.c_str();
  while (*line) {
    char key[16];
    double value;
    int skip = 0;
    int n = sscanf(line, "%16[a-z_]:%lf %n", key, &value, &skip);
    CHECK(n >= 2);    // invalid line
    CHECK(skip > 0);  // invalid line
    line += skip;
    CHECK(line - kvline.c_str() <= kvline.size());  // really a bug

    if (!strcmp(key, "longitude_deg"))  { longitude_deg  = value; continue; }
    if (!strcmp(key, "latitude_deg"))   { latitude_deg  = value; continue; }
    if (!strcmp(key, "angle_true_deg")) { angle_true_deg  = value; continue; }
    if (!strcmp(key, "mag_true_kn"))    { mag_true_kn  = value; continue; }
    CHECK(false);  // if we get here there is an unrecognized key.
  }
}

std::string SkipperInput::ToString() const {
  char line[1024];
  int s = snprintf(line, sizeof line,
      "longitude_deg:%f latitude_deg:%f angle_true_deg:%f mag_true_kn:%f\n",
      longitude_deg, latitude_deg, angle_true_deg, mag_true_kn);
  return std::string(line, s);
}

