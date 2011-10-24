// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef _IO_MTCP_H_
#define _IO__MTCP_H_

#include <stdint.h>

// API for the Xsens’ range of miniature MEMS based inertial Motion
// Trackers; the MTi‐G, the MTi and the MTx. These Motion Trackers (or MTs)
// all use a common binary communication protocol called the “MT Communication Protocol”
// as described in the 'Low Level Communication guide' that can be obtained here:
//    http://www.xsens.com/en/general/mti-g


 
enum {
	IMU_REQCONFIGURATION	= 0x0C, IMU_CONFIGURATION,
	IMU_RESTOREFACTORYDEF	= 0x0E, IMU_RESTOREFACTORYDEFACK,
	IMU_GOTOMEASUREMENT	= 0x10,	IMU_GOTOMEASUREMENTACK,
	IMU_GOTOCONFIG		= 0x30,	IMU_GOTOCONFIGACK,
	IMU_RESET		= 0x40,	IMU_RESETACK,

	IMU_OUTPUTMODE		= 0xD0, IMU_OUTPUTMODEACK,
	IMU_OUTPUTSETTINGS	= 0xD2, IMU_OUTPUTSETTINGSACK,
	IMU_OUTPUTSKIPFACTOR	= 0xD4, IMU_OUTPUTSKIPFACTORACK,
	IMU_LEVERARM		= 0x68, IMU_LEVERARMACK,
	IMU_SCENARIO		= 0x64, IMU_SCENARIOACK,

	IMU_MTDATA		= 0x32,
	IMU_WAKEUP		= 0x3E,	IMU_WAKEUPACK,
	IMU_ERROR		= 0x42,
};

enum {
	// Output mode, message id 0xD0, 16 bits
	IMU_OM_TMP = 1<<0,
	IMU_OM_CAL = 1<<1,
	IMU_OM_ORI = 1<<2,
	IMU_OM_AUX = 1<<3,
	IMU_OM_POS = 1<<4,
	IMU_OM_VEL = 1<<5,
	IMU_OM_STS = 1<<11,

	// can't combine with the others, don't use
	IMU_OM_GPS = 1<<12,
	IMU_OM_RAW = 1<<14,

	// Output settings, message id 0xD2
	// timestamp: 0 is none
	IMU_OS_TS_SC = 1<<0,   // timestamp: sample counter
	IMU_OS_TS_UTC = 1<<1,  // timestamp: utc time

	// orientation output mode, 0 is quaternions.  don't mix the following
	IMU_OS_OR_EULER = 1<<2,
	IMU_OS_OR_MATRIX = 1<<3,
	IMU_OS_OR_MASK = IMU_OS_OR_EULER|IMU_OS_OR_MATRIX,

	// calibr mode disable bits
	IMU_OS_CM_DISACC = 1<<4,
	IMU_OS_CM_DISGYR = 1<<5,
	IMU_OS_CM_DISMAG = 1<<6,

	// bit 7 reserved, 8,9 float format: use 0: default.
	IMU_OS_FF_MASK = (1<<8|1<<9),
	// bit 10, 11: aux mode
	IMU_OS_AU_DIS1 = (1<<10),
	IMU_OS_AU_DIS2 = (1<<11),

	//  rest: we don't use either
};

// User Guide section 2.2.6 and Comm.Guide section 4.3.7
enum {
	IMU_XKFSCENARIO_MARINE = 17,
	IMU_XKFSCENARIO_AEROSPACE_NOBARO = 10,
};

// we use: temp, calibr.data orientation data, position data velocity data, status data, timestamp=utc time
// orientation mode = euler, datafmt = standard, send acc, gyr and mag
// mt msg len = 82  (0xFA 0xFF 0x32 0x4D ...
enum {
	IMU_OUTPUT_MODE = IMU_OM_TMP|IMU_OM_CAL|IMU_OM_ORI|IMU_OM_POS|IMU_OM_VEL|IMU_OM_STS,
	IMU_OUTPUT_SETTINGS = IMU_OS_TS_UTC|IMU_OS_OR_EULER,
};


#endif // _IO_IMUD_MTCP_H_
