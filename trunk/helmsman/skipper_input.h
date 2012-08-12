// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#ifndef HELMSMAN_SKIPPER_INPUT_H
#define HELMSMAN_SKIPPER_INPUT_H

#include <string>
#include <stdint.h>

struct SkipperInput {
  SkipperInput();
  SkipperInput(const char* line);
  SkipperInput(int64_t timestamp_ms, double latitude_deg, double longitude_deg,
               double angle_true_deg, double mag_true_kn);
  void Reset();
  bool Valid();

  // Returns the number of read characters, 0 if line was in no valid format. 
  int FromString(const char* line);
  std::string ToString() const;

  bool operator!=(const SkipperInput r);

  int64_t timestamp_ms;
  // GPS data in degrees
  // Both values are nan initially.
  double latitude_deg;
  double longitude_deg;

  // true wind, global frame
  // Both values are nan initially.
  double angle_true_deg;
  double mag_true_kn;
};

#endif  // HELMSMAN_SKIPPER_INPUT_H
