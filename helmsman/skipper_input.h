// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#ifndef HELMSMAN_SKIPPER_INPUT_H
#define HELMSMAN_SKIPPER_INPUT_H

#include <string>

struct SkipperInput {
  SkipperInput();
  SkipperInput(const std::string& kvline);
  void Reset();
  std::string ToString() const;
  bool operator!=(const SkipperInput r);
  // GPS data in degrees
  // Both values are 0 initially.
  double longitude_deg;
  double latitude_deg;

  // true wind, global frame
  // Both values are kUnknown initially.  MAKE THEM math.h::NAN
  double angle_true_deg;
  double mag_true_kn;   // m/s?
};

#endif  // HELMSMAN_SKIPPER_INPUT_H
