// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SYSMGR_ADAM_H__
#define SYSMGR_ADAM_H__

#include <stdio.h>

#include "lib/util/serial.h"

// Interface to control ADAM-4068 relay module through serial port
class Adam {
 public:
  // Connect to device over serial port
  bool Init(const char *devname);

  // relay_id must be between 0 and 7 for ADAM-4068
  // state is true for relay on, false for relay off
  bool SetRelayState(unsigned int relay_id, bool state);
  bool GetRelayState(unsigned int relay_id, bool *state);

  // Create an off-on-off pulse on the relay output
  bool Pulse(unsigned int relay_id, int duration_s);

 private:
  Serial serial_;
};

#endif // SYSMGR_ADAM_H__
