// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_STORM_CONTROLLER_CONST_H
#define HELMSMAN_STORM_CONTROLLER_CONST_H

#include "common/convert.h"

// The smallest angle of attack where the sail does not flutter.
const double kSailStormAngleRad = Deg2Rad(8);
// The angle where the boat can best ride waves from behind.
const double kBoatStormAngleRad = Deg2Rad(15);

#endif  // HELMSMAN_STORM_CONTROLLER_CONST_H

