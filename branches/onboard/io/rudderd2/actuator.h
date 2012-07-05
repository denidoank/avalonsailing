// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef IO_RUDDERD2_ACTUATOR_H
#define IO_RUDDERD2_ACTUATOR_H

#include <stdint.h>

#include "ebus.h"

enum { LEFT = 0, RIGHT = 1, SAIL = 2, BMMH = 3 };

// Home is the extreme outer position for both rudders.  the
// controller reports '0'qc's for that.  We bracket the instructions
// to be between 0 and extreme_pos qc.  home_angle and extr angle will
// be (TODO were) determined by a manual method :-)
// for the sail and bmmh sensor (home, extreme) just defines the linear angle transformation
// TODO the sail is off by 3 degrees clockwise for bmmh == 0 mod 4096
typedef struct MotorParams MotorParams;
struct MotorParams {
        const char* label;
        uint32_t serial_number;    // of this epos.
        double home_angle_deg;     // angle when pos = home
        double extr_angle_deg;     // angle when pos = extreme
        int32_t home_pos_qc;       // zero of scale
        int32_t extr_pos_qc;       // max pos allowed (can be + or -)
};

extern MotorParams motor_params[];

int32_t angle_to_qc(MotorParams* p, double angle_deg);
double  qc_to_angle(MotorParams* p, int32_t pos_qc);

enum {
        REG_CONTROL = REGISTER(0x6040, 0),
                CONTROL_CLEARFAULT = 0x80,
                CONTROL_SHUTDOWN = 6,
                CONTROL_START = 0x3F,
                CONTROL_SWITCHON = 0xF,
        REG_STATUS  = REGISTER(0x6041, 0),
                STATUS_FAULT = (1<<3),
                STATUS_HOMEREF = (1<<15),
                STATUS_HOMINGERROR = (1<<13),
                STATUS_TARGETREACHED = (1<<10),
        REG_OPMODE  = REGISTER(0x6060, 0),
                OPMODE_HOMING = 6,
                OPMODE_PPM = 1,

	REG_ERROR = REGISTER(0x1001, 0),
	REG_ERRHISTCOUNT = REGISTER(0x1003, 0),

        REG_TARGPOS = REGISTER(0x607A, 0),
        REG_CURRPOS = REGISTER(0x6064, 0),

        REG_BMMHPOS = REGISTER(0x6004, 0),
};

#endif // IO_RUDDERD2_ACTUATOR_H
