// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_SAIL_CONTROLLER_CONST_H
#define HELMSMAN_SAIL_CONTROLLER_CONST_H

#include "lib/convert.h"

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

// Wait before switching to spinakker mode.
const double kSpinakkerSwitchDelay = 20;         // seconds

const double kTackHysteresis = Deg2Rad(35);      // for the tack side logic

// The maximum wind speed for the spinakker mode.
// The spinakker mode is used when we are running
// (wind from behind). If the wind is too strong the
// bow gets pushed down too much and the mast could
// be overloaded.
const double kSpinakkerLimit = 10;      // in m/s;
const double kAngleReductionLimit = 14; // in m/s


#endif  // HELMSMAN_SAIL_CONTROLLER_CONST_H

