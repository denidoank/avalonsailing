// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/adam.h"

#include <termios.h>
#include <unistd.h>

#include "lib/testing/pass_fail.h"

// Test port state changes of ADAM-4068 relay-controller
// agains real hardware
int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "missing serial device name\n");
    return 1;
  }

  Adam adam;
  bool state;

  PF_TEST(adam.Init(argv[1]), "open serial device");
  PF_TEST(adam.GetRelayState(0, &state), "get relay state 0");
  PF_TEST(state == false, "relay 0 is off");

  for (int i = 0; i < 8; i++) {
    fprintf(stderr, "testing relay port %d on\n", i);
    PF_TEST(adam.SetRelayState(i, true), "set relay state on");
    PF_TEST(adam.GetRelayState(i, &state), "get relay state");
    PF_TEST(state, "relay is on");
  }

  PF_TEST(adam.SetRelayState(9, true) == false, "set relay out of range test");
  PF_TEST(adam.GetRelayState(9, &state) == false,
          "get relay out of range test");

  sleep(1);

  for (int i = 0; i < 8; i++) {
    fprintf(stderr, "testing relay port %d off\n", i);
    PF_TEST(adam.SetRelayState(i, false), "set relay state off");
    PF_TEST(adam.GetRelayState(i, &state), "get relay state");
    PF_TEST(state == false, "relay is off");
  }

  PF_TEST(adam.Pulse(0, 1), "1s pulse on port 0");

  return 0;
}
