// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/recovery.h"

#include <unistd.h>

#include "lib/testing/pass_fail.h"


int main(int argc, char **argv) {
  RecoveryPolicy recovery(10, 3600);
  KeyValuePair config("recovery_time:\"2000-01-01T00:00:00\"");
  Alarm alarm("owner", "description", 0);
  PF_TEST(alarm.TrySet(), "alarm set");
  recovery.LoadState(config);
  PF_TEST(recovery.NextAuthorizedRecoveryTimeS(alarm) == 0,
          "recovery possible now");
  sleep(1);
  PF_TEST(recovery.StartRecovery(alarm), "start recovery");
  PF_TEST(recovery.IsRecovering(alarm), "this alarm is pre-recovery");

  PF_TEST(recovery.NextAuthorizedRecoveryTimeS(alarm) > 3000,
          "next recovery possible only in  about retry-time");
  alarm.TryClear();
  alarm.TrySet();
  PF_TEST(recovery.NextAuthorizedRecoveryTimeS(alarm) > 5 &&
          recovery.NextAuthorizedRecoveryTimeS(alarm) < 11,
          "next unrelated recovery after about repair-time");
}
