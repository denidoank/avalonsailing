// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "units.h"

#include <cassert>
#include <string>

using std::string;

// String representation of all units.
struct UnitToStringMapping {
  Unit unit;
  const char* as_string;
} kUnitToString[] = {
  // Speed.
  { UNIT_KNOTS,             "kn"  },
  { UNIT_METERS_PER_SECOND, "m_s" },
  { UNIT_NAUTICAL_MILES,    "nm"  },
  { UNIT_DEGREES,           "deg" },
  { UNIT_RADIANS,           "rad" },
  { UNIT_SECONDS,           "s"   },
  { UNIT_MILLISECONDS,      "ms"  },
  { UNIT_MICROSECONDS,      "us"  },
  { UNIT_CELSIUS,           "c"   },
  { UNIT_INVALID,           "NA"  }
};

bool isUnitMapValid() {
  for (int i = 0; i < UNIT_INVALID; ++i) {
    if (kUnitToString[i].unit != i) return false;
    if (string(kUnitToString[i].as_string).empty()) return false;
    if (string(kUnitToString[i].as_string) == "NA") return false;
  }
  if (string(kUnitToString[UNIT_INVALID].as_string) != "NA") return false;

  return true;
}

Unit unitFromString(const string& unit_as_string) {
  for (int i = 0; i < UNIT_INVALID; ++i) {
    if (unit_as_string.compare(kUnitToString[i].as_string) == 0) {
      assert(i == kUnitToString[i].unit);
      return kUnitToString[i].unit;
    }
  }
  return UNIT_INVALID;
}

string stringFromUnit(Unit unit) {
  if (unit < 0 || unit >= UNIT_INVALID) {
    return kUnitToString[UNIT_INVALID].as_string;
  }
  assert(kUnitToString[unit].unit == unit);
  return kUnitToString[unit].as_string;
}
