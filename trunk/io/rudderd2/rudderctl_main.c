// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Issue epos commands to keep one rudder (-l[eft] or -r[ight])
// homed and close to the reference value
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

// -----------------------------------------------------------------------------
// static const char* version = "$Id: $";
static const char* argv0;
static int verbose = 0;
static int debug = 0;

static void usage(void) {
	fprintf(stderr,	"usage: plug /path/to/ebus -- %s {-l | -r} \n", argv0);
	exit(2);
}

static Bus* bus = NULL;
static MotorParams* params = NULL;
static Device* dev = NULL;
static double target_angle_deg = NAN;

const double CLIP_DEG = 50;  // +/- max angle of operation when storm flag is off
const double TOLERANCE_DEG = .05;  // aiming precision in targetting rudder
const int64_t BUSLATENCY_WARN_THRESH_US = 100*1000;  // 100ms

// Read 1 line of input from stdin.
// rudderctl: we handle here, the epos messages are handled by bus_receive.
// return 1 if something (target_angle_deg or a register cache in the bus) changed, 0 otherwise.
static int processinput() {
	char line[1024];
	if (!fgets(line, sizeof(line), stdin))
		crash("reading stdin");

	if (line[0] == 'r') {  // 'rudderctl: ...

		struct RudderProto msg = INIT_RUDDERPROTO;
		int nn;
		int n = sscanf(line, IFMT_RUDDERPROTO_CTL(&msg, &nn));
		if (n != 5)
			return 0;

		if(msg.storm_flag)
			target_angle_deg = (params == &motor_params[LEFT]) ? 90.0 : -90.0;
		else {
			target_angle_deg = (params == &motor_params[LEFT]) ? msg.rudder_l_deg : msg.rudder_r_deg;
			if(target_angle_deg < -CLIP_DEG) target_angle_deg = -CLIP_DEG;
			if(target_angle_deg >  CLIP_DEG) target_angle_deg =  CLIP_DEG;
		}

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


// Work towards a state where the homed bit is set and we're in PPM mode
// Returns
//    DEFUNCT:  waiting for epos response to register query
//    HOMING:   in initalizing sequence
//    TARGETTING:  ready for rudder_control
static int rudder_init(void)
{
        int r;
        uint32_t status;
        uint32_t error;
        uint32_t control;
        uint32_t opmode;

        if (!device_get_register(dev, REG_STATUS, &status))
                return DEFUNCT;

        if (status & STATUS_FAULT) {
		slog(LOG_DEBUG, "rudder_init clearing fault 0x%x", status);
                device_invalidate_register(dev, REG_CONTROL);   // force re-issue
                device_set_register(dev, REG_CONTROL, CONTROL_CLEARFAULT);
                device_invalidate_register(dev, REG_ERROR);   // force re-issue
		device_get_register(dev, REG_ERROR, &error);  // we won't read it but eposmon will pick up the response.
                device_invalidate_register(dev, REG_STATUS);
                return DEFUNCT;
        }

        r  = device_get_register(dev, REG_CONTROL, &control);
        r &= device_get_register(dev, REG_OPMODE,  &opmode);

        if (!r) return DEFUNCT;

        int32_t minpos = (params->home_pos_qc < params->extr_pos_qc)
                ? params->home_pos_qc : params->extr_pos_qc;
        int32_t maxpos = (params->home_pos_qc > params->extr_pos_qc)
                ? params->home_pos_qc : params->extr_pos_qc;

        int32_t delta  = angle_to_qc(params, TOLERANCE_DEG);
	if(delta < 0) delta = -delta;
        minpos -= 10*delta;
        maxpos += 10*delta;
        int32_t method = (params->home_pos_qc < params->extr_pos_qc) ? 1 : 2;

        r &= device_set_register(dev, REGISTER(0x2080, 0),   500);  // homing current_threshold       User specific [500 mA]
        r &= device_set_register(dev, REGISTER(0x2081, 0),     0);  // home_position User specific [0 qc]
        r &= device_set_register(dev, REGISTER(0x6065, 0), 50*delta); // max_following_error User specific [2000 qc]
        r &= device_set_register(dev, REGISTER(0x6067, 0), delta);  // position window [qc], see 14.66
        r &= device_set_register(dev, REGISTER(0x6068, 0),    50);  // position time window [ms], see 14.66
        r &= device_set_register(dev, REGISTER(0x607C, 0),     0);  // home_offset User specific [0 qc]
        r &= device_set_register(dev, REGISTER(0x607D, 1), minpos); // min_position_limit User specific [-2147483648 qc]
        r &= device_set_register(dev, REGISTER(0x607D, 2), maxpos); // max_position_limit User specific [2147483647 qc]
        r &= device_set_register(dev, REGISTER(0x607F, 0), 25000);  // max_profile_velocity  Motor specific [25000 rpm]
        r &= device_set_register(dev, REGISTER(0x6081, 0),  3000);  // profile_velocity Desired Velocity [1000 rpm]
        r &= device_set_register(dev, REGISTER(0x6083, 0), 10000);  // profile_acceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6084, 0), 10000);  // profile_deceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6085, 0), 10000);  // quickstop_deceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6086, 0),     0);  // motion_profile_type  User specific [0]
        r &= device_set_register(dev, REGISTER(0x6098, 0), method); // homing_method           see firmware doc
        r &= device_set_register(dev, REGISTER(0x6099, 1),   200);  // switch_search_speed     User specific [100 rpm]
        r &= device_set_register(dev, REGISTER(0x6099, 2),    10);  // zero_search_speed       User specific [10 rpm]
        r &= device_set_register(dev, REGISTER(0x609A, 0),  1000);  // homing_acceleration     User specific [1000 rpm/s]

        if (!r) {
                device_invalidate_register(dev, REG_CONTROL);
                device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                return DEFUNCT;
        }
        slog(LOG_DEBUG, "rudder_init configured");

        if (!(status & STATUS_HOMEREF)) {
                if (opmode != OPMODE_HOMING) {
                        slog(LOG_DEBUG, "rudder_init set opmode homing");
                        device_set_register(dev, REG_OPMODE, OPMODE_HOMING);
                        device_invalidate_register(dev, REG_CONTROL);
                        device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                        device_invalidate_register(dev, REG_STATUS);
                        return HOMING;
                }

                switch (control) {
                case CONTROL_SHUTDOWN:
                        slog(LOG_DEBUG, "rudder_init homing, switchon");
                        device_set_register(dev, REG_CONTROL, CONTROL_SWITCHON);
                        break;

                case CONTROL_SWITCHON:
                        slog(LOG_DEBUG, "rudder_init homing, start");
                        device_set_register(dev, REG_CONTROL, CONTROL_START);
                        break;

                case CONTROL_START:
                        slog(LOG_DEBUG, "rudder_init homing, waiting");
                        if (!(status & STATUS_HOMINGERROR))
                                break;
                        slog(LOG_DEBUG, "rudder_init homing error: %x", status);
                        // fallthrough

                default:
                        slog(LOG_DEBUG, "rudder_init homing bad state: control %x, status %x", control, status);
                        device_invalidate_register(dev, REG_OPMODE);
                        device_set_register(dev, REG_OPMODE, OPMODE_HOMING);
                        device_invalidate_register(dev, REG_CONTROL);
                        device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                }

                device_invalidate_register(dev, REG_STATUS);
                return HOMING;

        } // not HOMEREF'ed

        slog(LOG_DEBUG, "rudder_init homeref ok.");

        if (opmode != OPMODE_PPM) {
                slog(LOG_DEBUG, "rudder_init set opmode PPM");
                device_set_register(dev, REG_OPMODE, OPMODE_PPM);
                device_invalidate_register(dev, REG_CONTROL);
                device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                device_invalidate_register(dev, REG_STATUS);
		return DEFUNCT;
        }

	return TARGETTING;
}

// Try to set the motion towards target_angle if neccesary
// Returns
//     DEFUNCT:  waiting for response to epos query
//     HOMING:   status FAULT or HOMEREF bit or PPM mode lost, please go execute rudder_init
//     TARGETTING:  in motion towards target
//     REACHED:   reached target angle to within tolerance
static int rudder_control(void)
{
        int r;
        uint32_t status;
        uint32_t error;
        uint32_t control;
        uint32_t opmode;
	int32_t curr_targ_qc;

        if (!device_get_register(dev, REG_STATUS, &status))
                return DEFUNCT;

        if (status & STATUS_FAULT) {
                slog(LOG_DEBUG, "rudder_control: clearing fault 0x%x", status);
                device_invalidate_register(dev, REG_CONTROL);   // force re-issue
                device_set_register(dev, REG_CONTROL, CONTROL_CLEARFAULT);
                device_invalidate_register(dev, REG_ERROR);   // force re-issue
		device_get_register(dev, REG_ERROR, &error);  // we won't read it but eposmon will pick up the response.
                device_invalidate_register(dev, REG_STATUS);
                return HOMING;
        }

	// these will be read from the bus register cache if possible
        r  = device_get_register(dev, REG_OPMODE,  &opmode);
        r &= device_get_register(dev, REG_CONTROL, &control);
        r &= device_get_register(dev, REG_TARGPOS, (uint32_t*)&curr_targ_qc);

        if (!r) return DEFUNCT;

	if (!(status & STATUS_HOMEREF) || opmode != OPMODE_PPM)
		return HOMING;

        slog(LOG_DEBUG, "rudder_control current target %dqc/%.3lf deg)",  curr_targ_qc, qc_to_angle(params, curr_targ_qc));

	if (isnan(target_angle_deg)) return REACHED;

        // no matter where we are, if current target is away from new target angle, we're not there
        double targ_diff = target_angle_deg - qc_to_angle(params, curr_targ_qc);
     
        if (targ_diff < 0) targ_diff = -targ_diff;
        if (targ_diff > TOLERANCE_DEG) {
                slog(LOG_DEBUG, "rudder_control must update target by %.3lf deg", targ_diff);
                status &= ~STATUS_TARGETREACHED;
                control &= ~0x30;  // if we started, pretend we're just switched on
                device_invalidate_register(dev, REG_CONTROL);
        }

        if (!(status & STATUS_TARGETREACHED)) {
                switch (control) {
                case CONTROL_SHUTDOWN:
                        slog(LOG_DEBUG, "rudder_control targetting, switchon");
                        device_set_register(dev, REG_CONTROL, CONTROL_SWITCHON);
                        break;

                case CONTROL_SWITCHON:
                        slog(LOG_DEBUG, "rudder_control setting target position %lf -> %lf", qc_to_angle(params, curr_targ_qc), target_angle_deg);
                        device_invalidate_register(dev, REG_TARGPOS);
                        device_set_register(dev, REG_TARGPOS, angle_to_qc(params, target_angle_deg));
                        device_set_register(dev, REG_CONTROL, CONTROL_START);
                        break;

                case CONTROL_START:
                case CONTROL_SWITCHON | 0x0010:
                        slog(LOG_DEBUG, "rudder_control targetting, waiting");
                        if (!(status & STATUS_HOMINGERROR))
				break;
			//fallthrough
                default:
                        slog(LOG_NOTICE, "rudder_control targetting bad state: control 0x%x, status 0x%x", control, status);
                        device_invalidate_register(dev, REG_CONTROL);
                        device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                }

                device_invalidate_register(dev, REG_STATUS);
                return TARGETTING;
        }

        slog(LOG_DEBUG, "rudder_control target reached, shutdown");
        device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);

        // we're homeref, position is close enough, and we powered off
        device_invalidate_register(dev, REG_STATUS);
        return REACHED;
}

// -----------------------------------------------------------------------------

const int64_t WARN_INTERVAL_US = 15 * 1000 * 1000; // 15 seconds
const int64_t MAX_INTERVAL_US = 15*60*1000 * 1000; // 15 minutes

int main(int argc, char* argv[]) {

	int ch;
	int dotimestamps = 0;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "dhlrTv")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
		case 'l': params = &motor_params[LEFT]; break;
		case 'r': params = &motor_params[RIGHT]; break;
		case 'v': ++verbose; break;
		case 'T': ++dotimestamps; break;
		case 'h': 
		default:
			usage();
		}
	}

	argv += optind;	argc -= optind;

	if (argc != 0 || !params) usage();

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");

	char label[1024];
	snprintf(label, sizeof label, "%s(%s)", argv0, params->label);
	openlog(label, debug?LOG_PERROR:0, LOG_LOCAL2);
	if(!debug) setlogmask(LOG_UPTO(LOG_NOTICE));

	// ask linebusd to install filters
	printf("$name %s\n", params->label);
	printf("$subscribe 0x%x\n", params->serial_number);
	printf("$subscribe rudderctl:\n");  // must match IFMT_RUDDER_CTL
	fflush(stdout);

	setlinebuf(stdout);
	bus = bus_new(stdout);	
	bus_enable_timestamp(bus, dotimestamps);
	dev = bus_open_device(bus, params->serial_number);

	int64_t last_reached = now_us();
	int64_t warn_interval = WARN_INTERVAL_US;
	int state = DEFUNCT;

	for (;;) {
		slog(LOG_WARNING, "Initializing rudder.");

		device_invalidate_all(dev);

		uint32_t dum;
		device_get_register(dev, REG_STATUS, &dum);  // Kick off communications

		while (state != TARGETTING)
			if (processinput())
				state = rudder_init();

		slog(LOG_WARNING, "Done initalizing rudder.");

		while (state != HOMING) {
			if (processinput())
				state = rudder_control();

			if (isnan(target_angle_deg))
			    continue;

			int64_t now = now_us();
			
			if (state == REACHED  || (now < last_reached)) {
				last_reached = now;
				warn_interval = WARN_INTERVAL_US;
			}

			if (now - last_reached > warn_interval) {
				slog(LOG_WARNING, "Unable to reach target %.3lf", target_angle_deg);
				last_reached = now;  // shut up warning
				if (warn_interval < MAX_INTERVAL_US) warn_interval *= 2;
			}
		}
	}

	crash("main loop exit");
	return 0;
}


