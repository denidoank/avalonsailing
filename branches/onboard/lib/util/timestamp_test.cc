// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/util/timestamp.h"

#include <stdlib.h>

#include "lib/testing/pass_fail.h"

#define TEST_TIME "2011-04-01T13:15:00"

int main(int argc, char **argv) {
  Timestamp ts;
  Timestamp ts2;
  // Force to use UTC - DST handling of mktime/localtime seems sketchy at best.
  // (DST depends on the date we are handling, not on whether the local TZ
  // currently is in DST)
  setenv("TZ", "UTC", 1);
  tzset();

  PF_TEST(ts2.FromIsoString(ts.ToIsoString().c_str()), "print/parse");
  PF_TEST(ts.SecondsSince(ts2) == 0, "equivalence test");

  PF_TEST(ts.FromIsoString(TEST_TIME), "parsing string");
  PF_TEST(ts.ToIsoString() == TEST_TIME, "conversion identical to original");

  PF_TEST(ts.FromIsoString("2000-01-01 xyz") == false, "invalid input");

  PF_TEST(ts.FromIsoString( "2001-01-01T01:01:01"), "parse string t1");
  PF_TEST(ts2.FromIsoString("2001-01-01T01:01:11"), "parse string t2");
  PF_TEST(ts.SecondsSince(ts2) == -10, "negative delta");
  PF_TEST(ts2.SecondsSince(ts) == 10, "positive delta");
}
