// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Units of the data that are produced or consumed by the various
// components of the system.

#ifndef COMMON_UNITS_H
#define COMMON_UNITS_H

#include <string>

// Supported units. Must start at 0, be contiguous and end with
// UNIT_INVALID, for the unit-lookup functions. If you add a
// unit here, also add it to the kUnitToString constant in units.cc.
enum Unit {
  // Speed
  UNIT_KNOTS = 0,
  UNIT_METERS_PER_SECOND,
  // Distance
  UNIT_NAUTICAL_MILES,
  // Angles
  UNIT_DEGREES,
  UNIT_RADIANS,
  // Time
  UNIT_SECONDS,
  UNIT_MILLISECONDS,
  UNIT_MICROSECONDS,
  UNIT_CELSIUS,
  // Should always be last.
  UNIT_INVALID
};

// Returns true if the internal unit map is valid.
bool isUnitMapValid();

// Returns the unit that corresponds to a string. Returns UNIT_INVALID if
// @unit_as_string does not match any valid unit.
Unit unitFromString(const std::string& unit_as_string);

// Returns the string representation of @unit.
std::string stringFromUnit(Unit unit);

#endif
