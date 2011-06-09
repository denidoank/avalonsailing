// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// control routines for rudders and sail.
#include "eposclient.h"

enum { LEFT = 0, RIGHT = 1, SAIL = 2, BMMH = 3 };

// Home is the extreme outer position for both rudders.  the
// controller reports '0'qc's for that.  We bracket the instructions
// to be between 0 and extreme_pos qc.  home_angle and extr angle will
// be (TODO were) determined by a manual method :-)
// for the sail and bmmh sensor (home, extreme) just defines the linear angle transformation
typedef struct MotorParams MotorParams;
struct MotorParams {
        const char* label;
        uint32_t serial_number;    // of this epos.
        double home_angle_deg;     // angle when pos = home
        double extr_angle_deg;     // angle when pos = extreme
        int32_t home_pos_qc;       // zero of scale
        int32_t extr_pos_qc;       // max pos allowed (can be + or -)
};

// return codes of XXX_control
enum { DEFUNCT = 0, HOMING, TARGETTING, REACHED };

// tries to keep rudder 'referenced to home pos' and angle close enough to target_angle
int rudder_control(Device* dev,  MotorParams* params, double target_angle_deg, double* actual_angle_deg);

// tries to keep sail angle close enough to target angle.
int sail_control(Device* motor, Device* bmmh,
		 MotorParams* motor_params, MotorParams* bmmh_params,
		 double target_angle_deg, double* actual_angle_deg);
