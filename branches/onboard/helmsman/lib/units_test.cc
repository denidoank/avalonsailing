// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "units.h"

#include <string>

#include "lib/testing/pass_fail.h"

using std::string;

void TestTableConsistency() {
  PF_TEST(isUnitMapValid(), "Unit map is valid.");
}

void TestValidLookups() {
  PF_TEST(stringFromUnit(UNIT_KNOTS) == "kn", "Knots are in table.");
  PF_TEST(stringFromUnit(UNIT_SECONDS) == "s", "Seconds are in table.");
  PF_TEST(stringFromUnit(UNIT_CELSIUS) == "c", "Celsius are in table.");
  PF_TEST(unitFromString("kn") == UNIT_KNOTS, "'kn' unit found.");
  PF_TEST(unitFromString("m_s") == UNIT_METERS_PER_SECOND, "'m_s' unit found.");
  PF_TEST(unitFromString("c") == UNIT_CELSIUS, "'c' unit found.");
}

void TestInvalidLookups() {
  PF_TEST(stringFromUnit(UNIT_INVALID) == "NA", "'Invalid' unit.");

  PF_TEST(unitFromString("foo") == UNIT_INVALID, "'foo' unit not found.");
  PF_TEST(unitFromString("m/s") == UNIT_INVALID, "'m/s' unit not found.");
  PF_TEST(unitFromString("NA") == UNIT_INVALID, "'NA' maps to invalid.");
}

int main(int argc, char **argv) {
  TestTableConsistency();
  TestValidLookups();
  TestInvalidLookups();
  return 0;
}
