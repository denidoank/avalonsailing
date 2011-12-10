// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include <assert.h>

#include "actuator.h"

struct MotorParams motor_params[] = {
//      { "LEFT",  0x09011145, 100.0, -80.0, 0, -288000 },
//      { "RIGHT", 0x09010537, -90.0,  90.0, 0,  288000 },
// limiting inner angle
        { "LEFT",  0x09011145, 100.0, -50.0, 0, -288000*150/180 },
        { "RIGHT", 0x09010537, -90.0,  50.0, 0,  288000*140/180 },
// sail and bmmh *must* be 360 degree ranges
        { "SAIL",  0x09010506,  -180, 180.0, 615000, -615000 },
        { "BMMH",  0x00001227,  -180, 180.0, 2048, -2048 }, // 4096 tics for a complete rotation
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

int32_t normalize_qc(MotorParams* p, int32_t qc) {
        assert(p->extr_angle_deg - p->home_angle_deg == 360.0);
        int32_t r = p->extr_pos_qc - p->home_pos_qc;
        if (r < 0) r = -r;
        int32_t max, min;
        if (p->home_pos_qc <  p->extr_pos_qc) {
          min = p->home_pos_qc; max = p->extr_pos_qc;
        } else {
          max = p->home_pos_qc; min = p->extr_pos_qc;
        }
        while(qc < min) qc += r;
        while(qc > max) qc -= r;
        return qc;
}
