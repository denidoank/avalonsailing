// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_SAIL_CONTROLLER_CONST_H
#define HELMSMAN_SAIL_CONTROLLER_CONST_H

#include "common/convert.h"

// Constants for the mode hysteresis logic
// 3m/s: 73.5, 6m/s 72.5, 9m/s: 72.5, 12m/s: 68.5 degrees
const double kSwitchpoint = Deg2Rad(72.5);       // degrees, middle of hysteresis
const double kHalfHysteresis = Deg2Rad(5);       // degrees, less than jibe zone/2
const double kDragMax = Deg2Rad(93);             // 93 degree because of slightly asymmetric C_drag-curve of the sail.
// For a race we might reduce the switch back delay to 4 s.
// For cruising we should not increase it beyond the
// skipper update period.
// According to the IEEE-RAM submission report, the wind information 
// varies by +-5 degrees even when filtered over 90s.
const double kSwitchBackDelay = 20;              // seconds, Do not switch back for 20s! 

const double kHalfHysteresisSign = Deg2Rad(25);  // for the sign logic

// The maximum wind speed for the spinakker mode
const double kSpinakkerLimit = 15;      // in m/s; The mast would be overloaded.
const double kAngleReductionLimit = 22; // in m/s


#endif  // HELMSMAN_SAIL_CONTROLLER_CONST_H

