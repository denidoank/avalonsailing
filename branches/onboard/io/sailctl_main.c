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

#include "proto/rudder.h"
#include "proto/skew.h"

#include "log.h"
#include "timer.h"
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
static double target_angle_deg = NAN;
static struct SkewProto skew = INIT_SKEWPROTO;

const double TOLERANCE_DEG = 1.0;
const int64_t BUSLATENCY_WARN_THRESH_US = 200*1000;  // 200ms

// Read 1 line of input from stdin.
// rudderctl: we handle here, the epos messages are handled by bus_receive.
// return 1 if something (target_angle_deg or a register cache in the bus) changed, 0 otherwise.
static int processinput() {
	char line[1024];
	if (!fgets(line, sizeof(line), stdin))
		crash("reading stdin");

	if (line[0] == 'r') {  // "rudderctl.."

		struct RudderProto msg = INIT_RUDDERPROTO;
		int nn;
		int n = sscanf(line, IFMT_RUDDERPROTO_CTL(&msg, &nn));
		if (n != IFMT_RUDDERPROTO_CTL_ITEMS)
			return 0;

		target_angle_deg = msg.sail_deg;

	} else 	if (line[0] == 's') {  // "skew:..."

		int nn;
		int n = sscanf(line, IFMT_SKEWPROTO(&skew, &nn));
		if (n != 2)
			return 0;

	} else {
		int64_t lat_us = bus_receive(bus, line);
		slog(LOG_DEBUG, "bus receive: %lld", lat_us);

		int to = bus_expire(bus);
		if(to) slog(LOG_WARNING, "timed out %d epos requests", to);

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
        uint32_t control;

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

        if(!device_get_register(motor, REG_CONTROL, &control))
		return DEFUNCT;

        r = device_set_register(motor, REG_OPMODE, OPMODE_PPM);

	int32_t tol = angle_to_qc(&motor_params[SAIL], TOLERANCE_DEG) -  angle_to_qc(&motor_params[SAIL], 0);
	if(tol < 0) tol = -tol;

        r &= device_set_register(motor, REGISTER(0x6065, 0), 0xffffffff); // max_following_error User specific [2000 qc]
        r &= device_set_register(motor, REGISTER(0x6067, 0), tol);        // position window [qc], see 14.66
        r &= device_set_register(motor, REGISTER(0x6068, 0), 50);      // position time window [ms], see 14.66
        r &= device_set_register(motor, REGISTER(0x607D, 1), 0x80000000); // min_position_limit [-2147483648 qc]
        r &= device_set_register(motor, REGISTER(0x607D, 2), 0x7fffffff); // max_position_limit  [2147483647 qc]
        r &= device_set_register(motor, REGISTER(0x607F, 0), 25000);  // max_profile_velocity  Motor specific [25000 rpm]
        r &= device_set_register(motor, REGISTER(0x6081, 0),  8000);  // profile_velocity Desired Velocity [1000 rpm]
        r &= device_set_register(motor, REGISTER(0x6083, 0), 10000);  // profile_acceleration User specific [10000 rpm/s]
        r &= device_set_register(motor, REGISTER(0x6084, 0), 10000);  // profile_deceleration User specific [10000 rpm/s]
        r &= device_set_register(motor, REGISTER(0x6085, 0), 10000);  // quickstop_deceleration User specific [10000 rpm/s]
        r &= device_set_register(motor, REGISTER(0x6086, 0), 0);      // motion_profile_type  User specific [0]
        // brake config
        r &= device_set_register(motor, REGISTER(0x2078, 2), (1<<12));  // output mask bit for brake
        r &= device_set_register(motor, REGISTER(0x2078, 3), 0);        // output polarity bits
        r &= device_set_register(motor, REGISTER(0x2079, 4), 12);       // output 12 -> signal 4
	r &= device_set_register(motor, REGISTER(0x2078, 1), 0);	// brake off !
                slog(LOG_DEBUG, "rudder_init registers set: %d", r);

        if (!r) {
                device_invalidate_register(motor, REG_CONTROL);   // force re-issue
                device_set_register(motor, REG_CONTROL, CONTROL_SHUTDOWN);
                device_invalidate_register(motor, REG_STATUS);
                return DEFUNCT;
        }

	// if we're here, all the settings have been issued and we must have issued the shutdown last time
	if (control == CONTROL_SHUTDOWN) {
                slog(LOG_DEBUG, "rudder_init final switchon");
		device_invalidate_register(motor, REG_CONTROL);
                device_set_register(motor, REG_CONTROL, CONTROL_SWITCHON);
                device_invalidate_register(motor, REG_STATUS);
		return DEFUNCT;
	}

	return TARGETTING;
}

// update targpos and issue start command
// returns:
//   DEFUNCT: wait for register response
//   HOMING: fault status reset, go and run init
//   TARGETTING: in motion, keep moving
//   REACHED: target reached.
int sail_control() 
{
        int r;
        uint32_t status;
        uint32_t error;
        uint32_t opmode;
	int32_t curr_targ_qc;
	int32_t new_targ_qc;

        if (!device_get_register(motor, REG_STATUS, &status))
                return DEFUNCT;

        if (status & STATUS_FAULT) {
                slog(LOG_DEBUG, "rudder_control: clearing fault 0x%x", status);
                device_invalidate_register(motor, REG_CONTROL);   // force re-issue
                device_set_register(motor, REG_CONTROL, CONTROL_CLEARFAULT);
                device_invalidate_register(motor, REG_ERROR);   // force re-issue
		device_get_register(motor, REG_ERROR, &error);  // we won't read it but eposmon will pick up the response.
                device_invalidate_register(motor, REG_STATUS);
                return HOMING;
        }

	// these will be read from the bus register cache if possible
        r  = device_get_register(motor, REG_OPMODE,  &opmode);
        r &= device_get_register(motor, REG_TARGPOS, (uint32_t*)&curr_targ_qc);

        if (!r) return DEFUNCT;

	if (opmode != OPMODE_PPM) return HOMING;
	if (isnan(skew.angle_deg)) return DEFUNCT;
	if (isnan(target_angle_deg)) return REACHED;

	double curr_targ_deg = qc_to_angle(&motor_params[SAIL], curr_targ_qc);
	double delta_targ_deg = target_angle_deg - skew.angle_deg - curr_targ_deg;
	while (delta_targ_deg < -180.0) delta_targ_deg += 360.0;
	while (delta_targ_deg >  180.0) delta_targ_deg -= 360.0;
	
	new_targ_qc = curr_targ_qc + angle_to_qc(&motor_params[SAIL], delta_targ_deg);

	if (new_targ_qc != curr_targ_qc) {
		// make sure the brake is off.  this will expire and be re-issued from the bus cache every five seconds
		if (!device_set_register(motor, REGISTER(0x2078, 1), 0))
			return DEFUNCT;

		slog(LOG_DEBUG, "target %.1f -> %.1f", curr_targ_deg, qc_to_angle(&motor_params[SAIL], new_targ_qc));
                status &= ~STATUS_TARGETREACHED;
                device_invalidate_register(motor, REG_CONTROL);
		device_set_register(motor, REG_TARGPOS, new_targ_qc);
		device_set_register(motor, REG_CONTROL, CONTROL_START);
        }

        device_invalidate_register(motor, REG_STATUS);
        return (status & STATUS_TARGETREACHED) ? REACHED : TARGETTING;
}


// -----------------------------------------------------------------------------

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

	if(setvbuf(stdout, NULL, _IOLBF, 0))
		syslog(LOG_WARNING, "Failed to make stdout line-buffered.");

	// ask linebusd to install filters
	printf("$name sail\n");
	printf("$subscribe 0x%x\n", motor_params[SAIL].serial_number);
	printf("$subscribe rudderctl:\n");  // must match IFMT_RUDDER_CTL
	printf("$subscribe skew:\n");  // must match IFMT_SKEW
	fflush(stdout);

	bus = bus_new(stdout);
	bus_enable_timestamp(bus, dotimestamps);
	motor = bus_open_device(bus, motor_params[SAIL].serial_number);

	struct Timer reach;
	memset(&reach, 0, sizeof reach);

	int state = DEFUNCT;

	for(;;) {
		slog(LOG_INFO, "Initializing sail.");

		device_invalidate_all(motor);
		uint32_t dum;

		device_get_register(motor, REG_STATUS, &dum);  // Kick off communications

		while (state != TARGETTING)
			if (processinput())
				state = sail_init();

		slog(LOG_INFO, "Done initalizing sail.");

		if(isnan(skew.angle_deg))
			slog(LOG_WARNING, "No skew angle input yet.");

		while (isnan(skew.angle_deg))
			processinput();

		slog(LOG_INFO, "Got skew angle %.2lf", skew.angle_deg);

		while (state != HOMING) {
			if (processinput())
				state = sail_control();
			
			if(debug>2) fprintf(stderr, "state now: %s\n", status[state]);

			if (isnan(skew.angle_deg)) {
				slog(LOG_WARNING, "Lost skew angle.");
			}
		
			if (isnan(target_angle_deg))
			    continue;
		
			switch (state) {
			case TARGETTING:
				if(!timer_running(&reach))
					timer_tick_now(&reach, 1);
				break;
			case REACHED:
				if(timer_running(&reach))
					timer_tick_now(&reach, 0);
				break;
			}

			if (reach.count % 200 == 0) {
				struct TimerStats stats;
				timer_stats(&reach, &stats);
				slog(LOG_INFO, "Target reached "  OFMT_TIMER_STATS(stats));
			}
		}
	}

	crash("main loop exit");
	return 0;
}


