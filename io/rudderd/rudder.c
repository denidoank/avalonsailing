// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Control routine for rudders and sail of avalon.
//
// Methods for controlling Maxon motors over eposclient according to
// the "Maxon Positioning Controller; Application Note "Device Programming"
//
// http://test.maxonmotor.com/docsx/Download/Product/Pdf/EPOS_Application_Note_Device_Programming_E.pdf
//
// Detailed reference: "Epos Positioning Controller Documentation Firmware Specification"
// http://test.maxonmotor.com/docsx/Download/Product/Pdf/EPOS_Firmware_Specification_E.pdf
//

#include "rudder.h"
#include <assert.h>
#include <math.h>


#ifdef DEBUG
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void
vlogf(const char* fmt, ...)
{
        va_list ap;
        char buf[1000];
        va_start(ap, fmt);
        vsnprintf(buf, 1000, fmt, ap);
        fprintf(stderr, "%s%s%s\n", buf,
                (errno) ? ": " : "",
                (errno) ? strerror(errno): "" );
        va_end(ap);
        return;
}
#define VLOGF(...) vlogf(__VA_ARGS__)
#else
#define VLOGF(...) do {} while (0)
#endif

const uint32_t MAX_CURRENT_MA     = 3000;
const double MAX_ABS_SPEED_DEG_S  = 30;
const double MAX_ABS_ACCEL_DEG_S2 = 500;

int32_t angle_to_qc(struct MotorParams* p, double angle_deg) {
        double alpha = (angle_deg - p->home_angle_deg) / (p->extr_angle_deg - p->home_angle_deg);
        if (alpha < 0.0) alpha = 0.0;
        if (alpha > 1.0) alpha = 1.0;
        return (1.0 - alpha) * p->home_pos_qc + alpha * p->extr_pos_qc;
}

double qc_to_angle(struct MotorParams* p, int32_t pos_qc) {
        double alpha = pos_qc - p->home_pos_qc;
        alpha /= p->extr_pos_qc - p->home_pos_qc;
        return (1.0 - alpha) * p->home_angle_deg + alpha * p->extr_angle_deg;
}

int32_t normalize_qc(struct MotorParams* p, int32_t qc) {
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

// Desired state:
// - get all state
// - Status::Homeref bit set
//            -> if not, opmode == HOMING && homing in progress
// - PositionTarget set to target_angle_deg
// - Status::TargetReached bit set
// - Powered off

int
rudder_control(Device* dev,  MotorParams* params, double target_angle_deg, double* actual_angle_deg)
{
        int r;
        uint32_t status;
        uint32_t control;
        uint32_t opmode;
        int32_t curr_pos_qc;
        int32_t curr_targ_qc;

        VLOGF("rudder_control(%s %f)", params->label, target_angle_deg);

        if (!device_get_register(dev, REG_STATUS, &status))
                return DEFUNCT;

        if (status & STATUS_FAULT) {
                VLOGF("rudder_control: clearing fault");
                device_invalidate_register(dev, REG_CONTROL);   // force re-issue
                device_set_register(dev, REG_CONTROL, CONTROL_CLEARFAULT);
                device_invalidate_register(dev, REG_STATUS);
                return DEFUNCT;
        }

        r  = device_get_register(dev, REG_CONTROL, &control);
        r &= device_get_register(dev, REG_OPMODE,  &opmode);
        r &= device_get_register(dev, REG_CURRPOS, (uint32_t*)&curr_pos_qc);
        r &= device_get_register(dev, REG_TARGPOS, (uint32_t*)&curr_targ_qc);

        if (!r) return DEFUNCT;

        VLOGF("rudder_control(%s) got state %x(%d %f) %x(%d %f)", params->label,
              curr_pos_qc, curr_pos_qc, qc_to_angle(params, curr_pos_qc),
              curr_targ_qc, curr_targ_qc, qc_to_angle(params, curr_targ_qc));

        int32_t minpos = (params->home_pos_qc < params->extr_pos_qc)
                ? params->home_pos_qc : params->extr_pos_qc;
        int32_t maxpos = (params->home_pos_qc > params->extr_pos_qc)
                ? params->home_pos_qc : params->extr_pos_qc;
        int32_t delta  = (maxpos - minpos) / 256;  // aim for 8 bits of resolution, see tolerance below.
        minpos -= 10*delta;
        maxpos += 10*delta;
        int32_t method = (params->home_pos_qc < params->extr_pos_qc) ? 1 : 2;  // TODO check 23/27

        r &= device_set_register(dev, REGISTER(0x2080, 0), 1000); // current_threshold       User specific [500 mA]
        r &= device_set_register(dev, REGISTER(0x2081, 0), 0); // home_position User specific [0 qc]
        r &= device_set_register(dev, REGISTER(0x6065, 0), 2*delta); // max_following_error User specific [2000 qc]
        r &= device_set_register(dev, REGISTER(0x6067, 0), delta);   // position window [qc], see 14.66
        r &= device_set_register(dev, REGISTER(0x6068, 0), 50);      // position time window [ms], see 14.66
        r &= device_set_register(dev, REGISTER(0x607C, 0), 0);       // home_offset User specific [0 qc]
        r &= device_set_register(dev, REGISTER(0x607D, 1), minpos); // min_position_limit User specific [-2147483648 qc]
        r &= device_set_register(dev, REGISTER(0x607D, 2), maxpos); // max_position_limit User specific [2147483647 qc]
        r &= device_set_register(dev, REGISTER(0x607F, 0), 15000); // max_profile_velocity  Motor specific [25000 rpm]
        r &= device_set_register(dev, REGISTER(0x6081, 0), 10000);  // profile_velocity Desired Velocity [1000 rpm]
        r &= device_set_register(dev, REGISTER(0x6083, 0), 10000);  // profile_acceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6084, 0), 10000);  // profile_deceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6085, 0), 10000);  // quickstop_deceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6086, 0), 0);      // motion_profile_type  User specific [0]
        r &= device_set_register(dev, REGISTER(0x6098, 0), method); // homing_method           see firmware doc
        r &= device_set_register(dev, REGISTER(0x6099, 1), 200); // switch_search_speed     User specific [100 rpm]
        r &= device_set_register(dev, REGISTER(0x6099, 2), 10); // zero_search_speed       User specific [10 rpm]
        r &= device_set_register(dev, REGISTER(0x609A, 0), 10000); // homing_acceleration     User specific [1000 rpm/s]

        if (!r) {
                device_invalidate_register(dev, REG_STATUS);
                device_invalidate_register(dev, REG_CONTROL);
                device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                return DEFUNCT;
        }
        VLOGF("rudder_control(%s) configured", params->label);

        *actual_angle_deg = NAN;

        if (!(status & STATUS_HOMEREF)) {
                if (opmode != OPMODE_HOMING) {
                        VLOGF("rudder_control(%s) set opmode homing", params->label);
                        device_set_register(dev, REG_OPMODE, OPMODE_HOMING);
                        device_invalidate_register(dev, REG_CONTROL);
                        device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                        device_invalidate_register(dev, REG_STATUS);
                        return HOMING;
                }

                switch (control) {
                case CONTROL_SHUTDOWN:
                        VLOGF("rudder_control(%s) homing, switchon", params->label);
                        device_set_register(dev, REG_CONTROL, CONTROL_SWITCHON);
                        break;

                case CONTROL_SWITCHON:
                        VLOGF("rudder_control(%s) homing, start", params->label);
                        device_set_register(dev, REG_CONTROL, CONTROL_START);
                        break;

                case CONTROL_START:
                        VLOGF("rudder_control(%s) homing, waiting", params->label);
                        if (!(status & STATUS_HOMINGERROR))
                                break;
                        VLOGF("rudder_control(%s) homing error: %x", params->label, status);
                        // fallthrough

                default:
                        VLOGF("rudder_control(%s) homing bad state: control %x, status %x",
                              params->label, control, status);
                        device_invalidate_register(dev, REG_OPMODE);
                        device_set_register(dev, REG_OPMODE, OPMODE_HOMING);
                        device_invalidate_register(dev, REG_CONTROL);
                        device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                }

                device_invalidate_register(dev, REG_STATUS);
                return HOMING;

        } // not HOMEREF'ed

        VLOGF("rudder_control: homeref ok.");
        *actual_angle_deg = qc_to_angle(params, curr_pos_qc);

        if (opmode != OPMODE_PPM) {
                VLOGF("rudder_control(%s) set opmode PPM", params->label);
                device_set_register(dev, REG_OPMODE, OPMODE_PPM);
                device_invalidate_register(dev, REG_CONTROL);
                device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                device_invalidate_register(dev, REG_STATUS);
                return TARGETTING;
        }

	if (isnan(target_angle_deg)) return REACHED;

        // no matter where we are, if current target is away from new target angle, we're not there
        const double tolerance = (1.0/256.0); // imagine we have about 8 bits or resolution
        double targ_diff = target_angle_deg - qc_to_angle(params, curr_targ_qc);
        targ_diff /= params->extr_angle_deg - params->home_angle_deg;
        if (targ_diff < 0) targ_diff = -targ_diff;
        if (targ_diff > tolerance) {
                VLOGF("rudder_control(%s) target reached, but wrong target", params->label);
                status &= ~STATUS_TARGETREACHED;
                control &= ~0x30;  // if we started, pretend we're just switched on
                device_invalidate_register(dev, REG_CONTROL);
        }

        // targetreached is set, but are we really there?
        if (status & STATUS_TARGETREACHED) {
                double actual_diff = qc_to_angle(params, curr_pos_qc) - qc_to_angle(params, curr_targ_qc);
                actual_diff /= params->extr_angle_deg - params->home_angle_deg;
                if (actual_diff < 0) actual_diff = -actual_diff;
                if (actual_diff > tolerance) {
                        VLOGF("rudder_control(%s) target reached, but actual is wrong", params->label);
                        status &= ~STATUS_TARGETREACHED;
                }
        }

        if (!(status & STATUS_TARGETREACHED)) {
                switch (control) {
                case CONTROL_SHUTDOWN:
                        VLOGF("rudder_control(%s) targetting, switchon", params->label);
                        device_set_register(dev, REG_CONTROL, CONTROL_SWITCHON);
                        break;

                case CONTROL_SWITCHON:
                        VLOGF("rudder_control(%s) setting target position %f -> %f",
                              params->label, qc_to_angle(params, curr_targ_qc), target_angle_deg);
                        device_invalidate_register(dev, REG_TARGPOS);
                        device_set_register(dev, REG_TARGPOS, angle_to_qc(params, target_angle_deg));
                        device_set_register(dev, REG_CONTROL, CONTROL_START);
                        device_invalidate_register(dev, REG_CURRPOS);
                        break;

                case CONTROL_START:
                case CONTROL_SWITCHON | 0x0010:
                        VLOGF("rudder_control(%s) targetting, waiting", params->label);
                        if (!(status & STATUS_HOMINGERROR))
                                break;
                        // fallthrough
                default:
                        VLOGF("rudder_control(%s) targetting bad state: control %x, status %x",
                              params->label, control, status);
                        device_invalidate_register(dev, REG_CURRPOS);
                        device_invalidate_register(dev, REG_OPMODE);
                        device_invalidate_register(dev, REG_CONTROL);
                        device_set_register(dev, REG_OPMODE, OPMODE_PPM);
                        device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                }

                device_invalidate_register(dev, REG_CURRPOS);
                device_invalidate_register(dev, REG_STATUS);
                device_get_register(dev, REG_STATUS, &status);
                return TARGETTING;
        }

        VLOGF("rudder_control(%s) target reached, shutdown", params->label);
        device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
        // we're homeref, position is close enough, and we (braked &) powered off

        device_invalidate_register(dev, REG_CURRPOS);
        device_invalidate_register(dev, REG_STATUS);
        return REACHED;
}

// -----------------------------------------------------------------------------
// tries to keep sail angle close enough to target angle.

int sail_control(Device* motor, Device* bmmh,
		 MotorParams* motor_params, MotorParams* bmmh_params,
		 double target_angle_deg, double* actual_angle_deg) {

        int r;
        uint32_t status;
        uint32_t error;
        uint32_t control;
        uint32_t opmode;
        int32_t curr_pos_qc;
        int32_t curr_targ_qc;
        int32_t curr_abspos_qc;

        VLOGF("sail_control(%f)", target_angle_deg);

        if (!device_get_register(motor, REG_STATUS, &status))
                return DEFUNCT;

        if (!device_get_register(motor, REG_ERROR, &error))
                return DEFUNCT;

        VLOGF("sail_control(%f) status 0x%x", target_angle_deg, status);
	if (error)
		VLOGF("sail_control: Error %d: 0x%x", i, error);

        if (status & STATUS_FAULT) {
                VLOGF("sail_control: clearing fault");
                device_invalidate_register(motor, REG_CONTROL);   // force re-issue
                device_set_register(motor, REG_CONTROL, CONTROL_CLEARFAULT);
                device_invalidate_register(motor, REG_STATUS);
                device_invalidate_register(motor, REG_ERROR);
                return DEFUNCT;
        }

        r  = device_get_register(motor, REG_CONTROL, &control);
        r &= device_get_register(motor, REG_OPMODE,  &opmode);
        r &= device_get_register(motor, REG_CURRPOS, (uint32_t*)&curr_pos_qc);
        r &= device_get_register(motor, REG_TARGPOS, (uint32_t*)&curr_targ_qc);
        {
                // bmmh position is 30 bit signed,0 .. 0x4000 0000 -> 0x2000 0000 => -0x2000 0000
                r &= device_get_register(bmmh, REG_BMMHPOS, (uint32_t*)&curr_abspos_qc);
                if (curr_abspos_qc >= (1<<29)) curr_abspos_qc -= (1<<30);
                curr_abspos_qc = normalize_qc(bmmh_params, curr_abspos_qc);
        }

        if (!r) {
                VLOGF("sail control: incomplete status\n");
                return DEFUNCT;
        }

        *actual_angle_deg = qc_to_angle(bmmh_params, curr_abspos_qc);

        VLOGF("sail_control(%f <- %f)", target_angle_deg, *actual_angle_deg);

        r = device_set_register(motor, REG_OPMODE, OPMODE_PPM);

        r &= device_set_register(motor, REGISTER(0x2080, 0), 3000); // current_threshold       User specific [500 mA]
        r &= device_set_register(motor, REGISTER(0x6065, 0), 0xffffffff); // max_following_error User specific [2000 qc]
        r &= device_set_register(motor, REGISTER(0x6067, 0), 2000);   // position window [qc], see 14.66
        r &= device_set_register(motor, REGISTER(0x6068, 0), 50);      // position time window [ms], see 14.66
        r &= device_set_register(motor, REGISTER(0x607D, 1), 0x80000000); // min_position_limit [-2147483648 qc]
        r &= device_set_register(motor, REGISTER(0x607D, 2), 0x7fffffff); // max_position_limit  [2147483647 qc]
        r &= device_set_register(motor, REGISTER(0x607F, 0), 15000); // max_profile_velocity  Motor specific [25000 rpm]
        r &= device_set_register(motor, REGISTER(0x6081, 0), 5000);  // profile_velocity Desired Velocity [1000 rpm]
        r &= device_set_register(motor, REGISTER(0x6083, 0), 10000);  // profile_acceleration User specific [10000 rpm/s]
        r &= device_set_register(motor, REGISTER(0x6084, 0), 10000);  // profile_deceleration User specific [10000 rpm/s]
        r &= device_set_register(motor, REGISTER(0x6085, 0), 10000);  // quickstop_deceleration User specific [10000 rpm/s]
        r &= device_set_register(motor, REGISTER(0x6086, 0), 0);      // motion_profile_type  User specific [0]
        // brake config
        r &= device_set_register(motor, REGISTER(0x2078, 2), (1<<12));  // output mask bit for brake
        r &= device_set_register(motor, REGISTER(0x2078, 3), 0);        // output polarity bits
        r &= device_set_register(motor, REGISTER(0x2079, 4), 12);       // output 12 -> signal 4

        if (!r) {
                device_invalidate_register(motor, REG_CONTROL);   // force re-issue
                device_set_register(motor, REG_CONTROL, CONTROL_SHUTDOWN);
                return DEFUNCT;
        }

	if (isnan(target_angle_deg)) return REACHED;

        VLOGF("sail_control: configuration ok target %sreached",
              (status & STATUS_TARGETREACHED) ? "NOT ": "" );

        // we should operate the motor to make this angle zero
        double target_delta_deg = target_angle_deg - *actual_angle_deg;
        while (target_delta_deg < -180.0) target_delta_deg += 360.0;
        while (target_delta_deg >  180.0) target_delta_deg -= 360.0;

        VLOGF("sail_control: target delta: %f", target_delta_deg);

        int32_t new_targ_qc = curr_pos_qc + angle_to_qc(motor_params, target_delta_deg);
        int32_t delta_targ_qc = new_targ_qc - curr_targ_qc;
        if ((delta_targ_qc < -1000) || (delta_targ_qc > 1000)) {
                // pretend we didn't arrive and that we're not moving
                // for the decision tree beow
                status &= ~STATUS_TARGETREACHED;
                if (control == CONTROL_START) {
                        device_invalidate_register(motor, REG_CONTROL);
                        control &= ~0x30;
                }
        }

        VLOGF("sail_control: curr_pos_qc: %d curr_targ_qc: %d new_targ_qc: %d delta_targ_qc: %d",
              curr_pos_qc, curr_targ_qc, new_targ_qc, delta_targ_qc);


        if (!(status & STATUS_TARGETREACHED)) {
                VLOGF("sail_control: Status not reached, going to %d", new_targ_qc);
                if (!device_set_register(motor, REGISTER(0x2078, 1), 0)) {
                        VLOGF("sail_control: wait for break off");
                        return TARGETTING;
                }
                VLOGF("sail_control: break off confirmed");

                switch(control) {
                case CONTROL_SHUTDOWN:
                        VLOGF("sail_control: shutdown->switchon");
                        device_set_register(motor, REG_CONTROL, CONTROL_SWITCHON);
                        break;
                case CONTROL_SWITCHON:
                        VLOGF("sail_control: switchon -> start");
                        device_invalidate_register(motor, REG_TARGPOS);
                        device_set_register(motor, REG_TARGPOS, new_targ_qc);
                        device_set_register(motor, REG_CONTROL, CONTROL_START);
                        break;
                case CONTROL_START:
                        VLOGF("sail_control: moving, patience");
                        break;
                default:  // weird, shutdown first
                        VLOGF("sail_control: ? (%x) -> shutdown", control);
                        device_set_register(motor, REG_CONTROL, CONTROL_SHUTDOWN);
                        break;
                }
        } else {
                VLOGF("sail_control: Status Reached");
                device_set_register(motor, REG_CONTROL, CONTROL_SHUTDOWN);
                if (device_set_register(motor, REGISTER(0x2078, 1), (1<<12)))  // brake on
                        VLOGF("sail_control: brake on");
        }

        device_invalidate_register(bmmh,  REG_BMMHPOS);
        device_invalidate_register(motor, REG_CURRPOS);
        device_invalidate_register(motor, REG_STATUS);
	device_invalidate_register(motor, REG_ERROR);
        return (status & STATUS_TARGETREACHED) ? REACHED : TARGETTING;
}
