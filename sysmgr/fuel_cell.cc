// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/fuel_cell.h"

#include <string.h>
#include <termios.h>
#include <stdio.h>

#define FC_TIMEOUT_MS 1000

bool FuelCell::Init(const char *devname) {
  // Default speed is 9600 bd
  return serial_.Init(devname, B9600);
}

bool FuelCell::GetOperatingInfo(FuelCell::FuelCellInfo *fci) {
  serial_.Printf("sfc\r");

  char buffer[100];
  int match_mask = 0;
  int max_data = 100;
  while(serial_.In().ReadLine(buffer, sizeof(buffer), FC_TIMEOUT_MS, '\r')
        == Reader::READ_OK && --max_data > 0) {
    float f;
    if (sscanf(buffer, "battery voltage %f1V", &f) == 1) {
      fci->voltage_mv = static_cast<long>(f * 1000);
      match_mask |= 1;
    } else if (sscanf(buffer, "output current %fA", &f) == 1) {
      fci->current_ma = static_cast<long>(f * 1000);
      match_mask |= 1 << 1;
    } else if (sscanf(buffer, "cumulative output energy %fWh", &f) == 1) {
      fci->energy_wh = static_cast<long>(f);
      match_mask |= 1 << 2;
    }

    if (match_mask == 7) {
      char c;
      max_data = 500;
      while(serial_.In().ReadChar(&c, 0)
            == Reader::READ_OK && --max_data > 0); // Eat up any extra input
      return true;
    }
  }
  return false;
}
