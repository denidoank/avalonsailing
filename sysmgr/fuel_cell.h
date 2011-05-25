// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SYSMGR_FUEL_CELL_H__
#define SYSMGR_FUEL_CELL_H__

#include "lib/util/serial.h"

// Class to interface with the diagnostic port of the fuel-cell
class FuelCell {
 public:
  struct FuelCellInfo {
    long voltage_mv; // current system supply voltage
    long current_ma; // output current
    long energy_wh; // cumulative energy output since last reset
  };

  // Connect to device over serial port
  bool Init(const char *devname);

  // Query some current operating values
  bool GetOperatingInfo(FuelCellInfo *fci);

 private:
  Serial serial_;
};

#endif // SYSMGR_FUEL_CELL_H__
