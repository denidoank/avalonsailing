// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Fuel cell status message.

#ifndef PROTO_FUELCELL_H
#define PROTO_FUELCELL_H

#include <math.h>

struct FuelcellProto {
  double tension_V;
  double charge_current_A;
  double energy_Wh;
  double runtime_h;
};

#define INIT_FUELCELLPROTO \
  {NAN, NAN, NAN, NAN}

// For use in printf and friends.
#define OFMT_FUELCELLPROTO(x, n)                                               \
	"fuelcell: tension_V:%.3lf charge_current_A:%.3lf energy_Wh:%.3lf "    \
        "runtime_h:%.3lf%n\n",                                                 \
	(x).tension_V, (x).charge_current_A, (x).energy_Wh, (x).runtime_h, (n)

#define IFMT_FUELCELLPROTO(x, n)                                               \
	"fuelcell: tension_V:%lf charge_current_A:%lf energy_Wh:%lf "          \
        "runtime_h:%lf%n",                                                     \
	&(x)->tension_V, &(x)->charge_current_A, &(x)->energy_Wh,              \
        &(x)->runtime_h, (n)

#endif // PROTO_FUELCELL_H
