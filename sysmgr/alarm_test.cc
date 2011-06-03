// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/alarm.h"

#include <stdio.h>

#include "lib/testing/pass_fail.h"

int main(int argc, char **argv) {
  Alarm alarm("owner", "alarm facility description", 0);

  PF_TEST(alarm.TrySet("false alarm..."), "first set");
  PF_TEST(alarm.TrySet() == false, "try set on alarm already set");
  fprintf(stderr, "%s\n", alarm.ToString().c_str());
  PF_TEST(alarm.TryClear(), "clearing of set alarm");
  fprintf(stderr, "%s\n", alarm.ToString().c_str());
}
