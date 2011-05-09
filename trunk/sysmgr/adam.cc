// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

// Driver for the Advantech ADAM-4068 relay control module
// See ftp.bb-elec.com/bb-elec/literature/manuals/Advantech/ADAM-4000.pdf
// for more detailed documentation and command reference.
//
// The device is assumed to be in default settings (after INIT* state)

#include "sysmgr/adam.h"

#include <string.h>
#include <termios.h>
#include <unistd.h>

#define ADAM_TIMEOUT_MS 500

bool Adam::Init(const char *devname) {
  // Default speed is 9600 bd
  return serial_.Init(devname, B9600);
}

bool Adam::SetRelayState(unsigned int relay_id, bool state) {
  if (relay_id >= 8) {
    return false;
  }
  // #AAPPDD<CR> command to set output of port PP to DD on module AA.
  // Single port addressing format is 1<p>, where p is the output port number.
  // Address hard-coded to module '01'.
  serial_.Printf("#011%d0%d\r", relay_id, state ? 1 : 0);
  char buffer[2];
  if (serial_.In().ReadLine(buffer, sizeof(buffer), ADAM_TIMEOUT_MS, '\r')
      != Reader::READ_OK) {
    return false;
  }
  // Expect '><cr>' for success
  return strcmp(buffer, ">") == 0;
}

bool Adam::GetRelayState(unsigned int relay_id, bool *state) {
  if (relay_id >= 8) {
    return false;
  }
  char buffer[10];
  // $AA6<CR> command to get state of all output ports.
  // Address hard-coded to module '01'.
  serial_.Printf("$016\r");
  if (serial_.In().ReadLine(buffer, sizeof(buffer), ADAM_TIMEOUT_MS, '\r')
      != Reader::READ_OK) {
    return false;
  }
  // Expect '!DD0000' for success, where DD is a hex number of the
  // relay port states (0: off/1: on)
  if (buffer[0] != '!') {
    return false;
  }
  unsigned long mask;
  if (sscanf(buffer, "!%lx", &mask) != 1) {
    return false;
  }
  mask >>= 16; // ignore 4 trailing hex characters
  *state = mask & (1 << relay_id);
  return true;
}

bool Adam::Pulse(unsigned int relay_id, int duration_s) {
  if (relay_id >= 8) {
    return false;
  }
  bool status = SetRelayState(relay_id, true);
  sleep(duration_s);
  return status & SetRelayState(relay_id, false);
}
