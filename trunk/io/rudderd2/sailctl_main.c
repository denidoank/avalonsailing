// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Issue epos commands to control the sail
// 

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "../../proto/rudder.h"

#include "../log.h"
#include "../timer.h"
#include "actuator.h"
#include "eposclient.h"

// static const char* version = "$Id: $";
static const char* argv0;
static int verbose = 0;
static int debug = 0;

static void usage(void) {
	fprintf(stderr,	"usage: plug /path/to/ebus | %s\n", argv0);
	exit(2);
}

static Bus* bus = NULL;
static Device* motor;
static Device* bmmh;
static double target_angle_deg = NAN;
static int storm_flag = 0;

const double TOLERANCE_DEG = 1.0;
const int64_t BUSLATENCY_WARN_THRESH_US = 100*1000;  // 100ms

// Read 1 line of input from stdin.
// rudderctl: we handle here, the epos messages are handled by bus_receive.
// return 1 if something (target_angle_deg or a register cache in the bus) changed, 0 otherwise.
static int processinput() {
	char line[1024];
	if (!fgets(line, sizeof(line), stdin))
		crash("reading stdin");

	if (line[0] == 'r') {  // 'rudderctl..

		struct RudderProto msg = INIT_RUDDERPROTO;
		int nn;
		int n = sscanf(line, IFMT_RUDDERPROTO_CTL(&msg, &nn));
		if (n != 5)
			return 0;

		target_angle_deg = msg.sail_deg;
		storm_flag = msg.storm_flag;

	} else {

		int to = bus_expire(bus);
		if(to) slog(LOG_WARNING, "timed out %d epos requests", to);

		int64_t lat_us = bus_receive(bus, line);
		if (lat_us == 0)
			return 0;
		if(lat_us > BUSLATENCY_WARN_THRESH_US)
			slog(LOG_WARNING, "high epos latency: %lld ms", lat_us/1000);

	}
	return 1;
}

enum { DEFUNCT = 0, HOMING, TARGETTING, REACHED };
const char* status[] = { "DEFUNCT", "HOMING", "TARGETTING", "REACHED" };


// Set config and opmode
// Returns
//	DEFUNCT:  waiting for response to epos query
//	TARGETTING:  ready for sail_control
int sail_init() {

        int r;
        uint32_t status;
        uint32_t error;

        slog(LOG_DEBUG, "sail_init");

        if (!device_get_register(motor, REG_STATUS, &status))
                return DEFUNCT;

        if (status & STATUS_FAULT) {
                slog(LOG_DEBUG, "sail_control: clearing fault");
                device_invalidate_register(motor, REG_CONTROL);   // force re-issue
                device_set_register(motor, REG_CONTROL, CONTROL_CLEARFAULT);
		device_invalidate_register(motor, REG_ERROR);   // force re-issue
		device_get_register(motor, REG_ERROR, &error);  // we won't read it but eposmon will pick up the response.
                device_invalidate_register(motor, REG_STATUS);
                return DEFUNCT;
        }

        r = device_set_register(motor, REG_OPMODE, OPMODE_PPM);

	int32_t tol = angle_to_qc(&motor_params[SAIL], TOLERANCE_DEG) -  angle_to_qc(&motor_params[SAIL], 0);
	if(tol < 0) tol = -tol;

        r &= device_set_register(motor, REGISTER(0x6065, 0), 0xffffffff); // max_following_error User specific [2000 qc]
        r &= device_set_register(motor, REGISTER(0x6067, 0), tol);        // position window [qc], see 14.66
        r &= device_set_register(motor, REGISTER(0x6068, 0), 50);      // position time window [ms], see 14.66
        r &= device_set_register(motor, REGISTER(0x607D, 1), 0x80000000); // min_position_limit [-2147483648 qc]
        r &= device_set_register(motor, REGISTER(0x607D, 2), 0x7fffffff); // max_position_limit  [2147483647 qc]
        r &= device_set_register(motor, REGISTER(0x607F, 0), 25000); // max_profile_velocity  Motor specific [25000 rpm]
        r &= device_set_register(motor, REGISTER(0x6081, 0), 8000);  // profile_velocity Desired Velocity [1000 rpm]
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

	return TARGETTING;
}

// update 
int sail_control(double* actual_angle_deg) {

        int r;
        uint32_t status;
        uint32_t error;
        uint32_t control;
        uint32_t opmode;
        int32_t curr_pos_qc;
        int32_t curr_targ_qc;
        int32_t curr_abspos_qc;

        slog(LOG_DEBUG, "sail_control(%f)", target_angle_deg);

        if (!device_get_register(motor, REG_STATUS, &status))
                return DEFUNCT;

	slog(LOG_DEBUG, "sail_control(%f) status 0x%x", target_angle_deg, status);

        if (status & STATUS_FAULT) {
                slog(LOG_DEBUG, "sail_control: clearing fault");
                device_invalidate_register(motor, REG_CONTROL);   // force re-issue
                device_set_register(motor, REG_CONTROL, CONTROL_CLEARFAULT);
		device_invalidate_register(motor, REG_ERROR);   // force re-issue
		device_get_register(motor, REG_ERROR, &error);  // we won't read it but eposmon will pick up the response.
                device_invalidate_register(motor, REG_STATUS);
                return HOMING;
        }
	
        r  = device_get_register(motor, REG_OPMODE,  &opmode);
        r &= device_get_register(motor, REG_CONTROL, &control);
        r &= device_get_register(motor, REG_CURRPOS, (uint32_t*)&curr_pos_qc);
        r &= device_get_register(motor, REG_TARGPOS, (uint32_t*)&curr_targ_qc);
        {
                // bmmh position is 30 bit signed,0 .. 0x4000 0000 -> 0x2000 0000 => -0x2000 0000
                r &= device_get_register(bmmh, REG_BMMHPOS, (uint32_t*)&curr_abspos_qc);
                if (curr_abspos_qc >= (1<<29)) curr_abspos_qc -= (1<<30);
                curr_abspos_qc = normalize_qc(&motor_params[BMMH], curr_abspos_qc);
        }

        if (!r) return DEFUNCT;

	if (opmode != OPMODE_PPM)
		return HOMING;

        *actual_angle_deg = qc_to_angle(&motor_params[BMMH], curr_abspos_qc);

        slog(LOG_DEBUG, "sail_control(%f <- %f)", target_angle_deg, *actual_angle_deg);

	if (isnan(target_angle_deg)) return REACHED;

        slog(LOG_DEBUG, "sail_control: configuration ok target %sreached",
              (status & STATUS_TARGETREACHED) ? "NOT ": "" );

        // we should operate the motor to make this angle zero
        double target_delta_deg = target_angle_deg - *actual_angle_deg;
        while (target_delta_deg < -180.0) target_delta_deg += 360.0;
        while (target_delta_deg >  180.0) target_delta_deg -= 360.0;

        slog(LOG_DEBUG, "sail_control: target delta: %f", target_delta_deg);

	int32_t new_targ_qc   = curr_pos_qc 
	  + angle_to_qc(&motor_params[SAIL], target_delta_deg)
	  - angle_to_qc(&motor_params[SAIL], 0);
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

        slog(LOG_DEBUG, "sail_control: curr_pos_qc: %d curr_targ_qc: %d new_targ_qc: %d delta_targ_qc: %d",
              curr_pos_qc, curr_targ_qc, new_targ_qc, delta_targ_qc);

        if (!(status & STATUS_TARGETREACHED)) {
                slog(LOG_DEBUG, "sail_control: Status not reached, going to %d", new_targ_qc);
                if (!device_set_register(motor, REGISTER(0x2078, 1), 0)) {
                        slog(LOG_DEBUG, "sail_control: wait for break off");
                        return TARGETTING;
                }
                slog(LOG_DEBUG, "sail_control: break off confirmed");

                switch(control) {
                case CONTROL_SHUTDOWN:
                        slog(LOG_DEBUG, "sail_control: shutdown->switchon");
                        device_set_register(motor, REG_CONTROL, CONTROL_SWITCHON);
                        break;

                case CONTROL_SWITCHON:
                        slog(LOG_DEBUG, "sail_control: switchon -> start");
                        device_invalidate_register(motor, REG_TARGPOS);
                        device_set_register(motor, REG_TARGPOS, new_targ_qc);
                        device_set_register(motor, REG_CONTROL, CONTROL_START);
                        break;

                case CONTROL_START:
                        slog(LOG_DEBUG, "sail_control: moving, patience");
                        break;

                default:  // weird, shutdown first
                        slog(LOG_DEBUG, "sail_control: ? (%x) -> shutdown", control);
                        device_set_register(motor, REG_CONTROL, CONTROL_SHUTDOWN);
                        break;
                }
        } else {
                slog(LOG_DEBUG, "sail_control: Status Reached");
                device_set_register(motor, REG_CONTROL, CONTROL_SHUTDOWN);

		if(!storm_flag)
			if (device_set_register(motor, REGISTER(0x2078, 1), (1<<12)))  // brake on
			  slog(LOG_DEBUG, "sail_control: brake on");
        }

        device_invalidate_register(bmmh,  REG_BMMHPOS);
        device_invalidate_register(motor, REG_CURRPOS);
        device_invalidate_register(motor, REG_STATUS);
        return (status & STATUS_TARGETREACHED) ? REACHED : TARGETTING;
}

// -----------------------------------------------------------------------------

const int64_t WARN_INTERVAL_US =   30*1000*1000;  // 30 seconds
const int64_t MAX_INTERVAL_US = 15*60*1000*1000;  // 15 minutes

int main(int argc, char* argv[]) {

	int ch;
	int dotimestamps = 0;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "dhTv")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
		case 'v': ++verbose; break;
		case 'T': ++dotimestamps; break;
		case 'h': 
		default:
			usage();
		}
	}

	argv += optind;	argc -= optind;
	if (argc != 0) usage();

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");

	openlog(argv0, debug?LOG_PERROR:0, LOG_LOCAL2);
	if(!debug) setlogmask(LOG_UPTO(LOG_NOTICE));

	// ask linebusd to install filters
	printf("$name sail\n");
	printf("$subscribe 0x%x\n", motor_params[SAIL].serial_number);
	printf("$subscribe 0x%x\n", motor_params[BMMH].serial_number);
	printf("$subscribe rudderctl:\n");  // must match IFMT_RUDDER_CTL
	fflush(stdout);

	setlinebuf(stdout);
	bus = bus_new(stdout);
	bus_enable_timestamp(bus, dotimestamps);
	motor = bus_open_device(bus, motor_params[SAIL].serial_number);
	bmmh  = bus_open_device(bus, motor_params[BMMH].serial_number);

	int64_t last_reached = now_us();
	int64_t warn_interval = WARN_INTERVAL_US;
	double angle_deg = NAN;
	int state = DEFUNCT;

	for(;;) {
		slog(LOG_WARNING, "Initializing sail.");

		device_invalidate_all(motor);
		device_invalidate_all(bmmh);
		uint32_t dum;

		device_get_register(motor, REG_STATUS, &dum);  // Kick off communications

		while (state != TARGETTING)
			if (processinput())
				state = sail_init();

		slog(LOG_WARNING, "Done initalizing sail.");

		while (state != HOMING) {
			if (processinput())
				state = sail_control(&angle_deg);

			if (isnan(target_angle_deg))
			    continue;

			int64_t now = now_us();
			
			if (state == REACHED  || (now < last_reached)) {
				last_reached = now;
				warn_interval = WARN_INTERVAL_US;
			}

			if (now - last_reached > warn_interval) {
				slog(LOG_WARNING, "Unable to reach target %.2lf actual %.2lf", target_angle_deg, angle_deg);
				last_reached = now;  // shut up warning
				if (warn_interval < MAX_INTERVAL_US) warn_interval *= 2;
			}
		}
	}

	crash("main loop exit");
	return 0;
}


