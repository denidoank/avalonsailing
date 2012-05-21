// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "actuator.h"

#include <assert.h>

struct MotorParams motor_params[] = {
#if 0
	{ "LEFT",  0x09011145, 101.0, -79.0, 0, -288000 },
	{ "RIGHT", 0x09010537, -97.0,  83.0, 0,  288000 },
#else  // limiting inner angle
        { "LEFT",  0x09011145, 101.0, -50.0, 0, -288000*151/180 },
        { "RIGHT", 0x09010537, -97.0,  50.0, 0,  288000*147/180 },
#endif
	// sail and bmmh *must* be -180..180 degree ranges
	// TODO the sail is off by 3 degrees clockwise for bmmh == 0 mod 4096
        { "SAIL",  0x09010506,  -180.0, 180.0, 615000, -615000 },
        { "BMMH",  0x00001227,  -180.0, 180.0, 2048, -2048 }, // 4096 tics for a complete rotation
};

int32_t angle_to_qc(MotorParams* p, double angle_deg) {
        double alpha = (angle_deg - p->home_angle_deg) / (p->extr_angle_deg - p->home_angle_deg);
        if (alpha < 0.0) alpha = 0.0;
        if (alpha > 1.0) alpha = 1.0;
        return (1.0 - alpha) * p->home_pos_qc + alpha * p->extr_pos_qc;
}

double qc_to_angle(MotorParams* p, int32_t pos_qc) {
        double alpha = pos_qc - p->home_pos_qc;
        alpha /= p->extr_pos_qc - p->home_pos_qc;
        return (1.0 - alpha) * p->home_angle_deg + alpha * p->extr_angle_deg;
}
