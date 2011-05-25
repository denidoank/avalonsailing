// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/fuel_cell.h"

#include <stdio.h>

#include "lib/testing/pass_fail.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "missing serial device name\n");
    return 1;
  }

  FuelCell fc;
  PF_TEST(fc.Init(argv[1]), "open fuel-cell serial device");
  FuelCell::FuelCellInfo fci;
  for (int i = 0; i < 3; i++) {
    PF_TEST(fc.GetOperatingInfo(&fci), "read fuel-cell status");
    printf("U: %ldmV, I: %ldmA, E: %ldWh\n", fci.voltage_mv,
           fci.current_ma, fci.energy_wh);
  }
}
