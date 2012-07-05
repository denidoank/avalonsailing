// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// Sail brake status is managed by the drive interface.
// All angle values in degrees. For the drives the [-180, 180] convention
// is used.
// Constructors, Reset, Check and conversion functions are provided for.

#ifndef HELMSMAN_DRIVE_DATA_H
#define HELMSMAN_DRIVE_DATA_H

#include <stdint.h>
#include <string>
#include "proto/rudder.h"


struct DriveReferenceValuesRad;

struct DriveActualValues {
  DriveActualValues();
  //DriveActualValues(const DriveActualValuesRad&);
  void Reset();
  void Check();
  double gamma_rudder_left_deg;
  double gamma_rudder_right_deg;
  double gamma_sail_deg;
  // homed == "ready to be used". If false, the commands will be ignored.
  bool homed_rudder_left;
  bool homed_rudder_right;
  bool homed_sail;
};

// All values in degrees. For the drives the [-180, 180] convention is used.
// The drive interface handles the wrap around from -179 to 179 degrees.
struct DriveReferenceValues {
  DriveReferenceValues();
  DriveReferenceValues(const DriveReferenceValuesRad& ref_rad);
  std::string ToString() const;
  void Reset();
  void Check();
  double gamma_rudder_star_left_deg;
  double gamma_rudder_star_right_deg;
  double gamma_sail_star_deg;
};

// Internally used radians versions.
struct DriveActualValuesRad {
  DriveActualValuesRad();
  DriveActualValuesRad(const DriveActualValues& act_deg);
  void Reset();
  std::string ToString() const;

  void FromProto(const RudderProto& status);

  void ToProto(RudderProto* status,
               int64_t timestamp_ms) const;

  double gamma_rudder_left_rad;
  double gamma_rudder_right_rad;
  double gamma_sail_rad;
  // homed == "ready to be used". If false, the commands will be ignored.
  bool homed_rudder_left;
  bool homed_rudder_right;
  bool homed_sail;
};

// All values in degrees. For the drives the [-180, 180] convention is used.
// The drive interface handles the wrap around from -179 to 179 degrees.
struct DriveReferenceValuesRad {
  DriveReferenceValuesRad();
  DriveReferenceValuesRad(const DriveReferenceValues& ref_deg);
  void Reset();
  void Check();
  std::string ToString() const;
  void FromProto(const RudderProto& sts);
  void ToProto(RudderProto* sts) const;

  double gamma_rudder_star_left_rad;
  double gamma_rudder_star_right_rad;
  double gamma_sail_star_rad;
};

#endif  // HELMSMAN_DRIVE_DATA_H
