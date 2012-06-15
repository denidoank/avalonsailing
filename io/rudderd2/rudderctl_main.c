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

const double TOLERANCE_DEG = .05;  // aiming precision in targetting rudder
const int64_t BUSLATENCY_WARN_THRESH_US = 200*1000;  // 200ms

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

		target_angle_deg = (params == &motor_params[LEFT]) ? msg.rudder_l_deg : msg.rudder_r_deg;

	} else {
		int64_t lat_us = bus_receive(bus, line);

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
        // See [EPOS-FirmwareSpecification-E.pdf, page 24ff]
	// We have no limit switch, we have a home switch but modes 11 and 7 don't work.
        int32_t method = (params->home_pos_qc < params->extr_pos_qc) ? 1 : 2;

	/*
	See Application Note Device Programming, page 3
        4 undefined parameters for rudder and sail each:
	Motor Type, Continuous Current Limit, Pole Pair Number, Thermal Time Constant Winding
	0x6402-00 0x6410-01 0x6410-03 0x6410-05
	Motor specific [10] Motor specific [5000] Motor specific [1] Motor specific [40]
	[Endbericht.pdf]:
	Rudder motor type: Maxon EC45 (250W, 24V, 136210) with GP 52 C gear, Encoder
	Motor type EC
	pole pairs: 1
	nom. continuous current 12.5A,
	thermal time constant winding: 31s
	nominal speed 7970rpm, no load: 8670rpm

	The GP52 C has the following transmission ratio options: 1 :
	4.3, 18.8, 81.4 or 352.6 (These are planetary gears so the can have odd transmission ratios)


	Sail motor type: Maxon EC60 (400W, 48V, 167132) with GP 81 A gear, Encoder and Baumer BMH ???? Absolute Position encoder, multiturn on mast axis
	http://www.specamotor.com/de/Maxon/motors/EC-60-167131/
	There are 2 winding options 167131 (nominal speed 2680rpm at 48V)
	and 167132 (4960rpm at 48V, 400W)
	We've got 24V, so we can assume that we are using the latter (167132) type

	motor type EC
	pole pairs: 1
	nom. current 9.32A
	thermal time constant: 33.7s
	nominal speed 2480rpm at 24V

	The GP81A has the following transmission ratio options: 1: 
	3.7, 13.7, 25.0, 50.9, 92.7, or 307.6

        TODOs:
	What are the 4 parameters set to currently?
        Speed limit for sail to 2500rpm.
	*/

        r &= device_set_register(dev, REGISTER(0x6410, 1),  5000);
        // The EPOS inverter makes 5A. (The motors continous current limit for EC45 is actually 12500,
        // but higher values cause a write error.), enough to pull out a wedged rudder.
        // This current determines the torque applied when should we ever jam the rudders edge at the hull.
	// It also sets the threshold when friction in the rudder gear is interpreted as a mechanical limit. 
        r &= device_set_register(dev, REGISTER(0x2080, 0),  1000);  // homing current_threshold       User specific [500 mA],
        r &= device_set_register(dev, REGISTER(0x2081, 0),     0);  // home_position User specific [0 qc]
        r &= device_set_register(dev, REGISTER(0x6065, 0), 50*delta); // max_following_error User specific [2000 qc] We dont want errors so we better increase this
        r &= device_set_register(dev, REGISTER(0x6067, 0), delta);  // position window [qc], see 14.66
        r &= device_set_register(dev, REGISTER(0x6068, 0),    50);  // position time window [ms], see 14.66
        r &= device_set_register(dev, REGISTER(0x607C, 0),     0);  // home_offset User specific [0 qc]
        r &= device_set_register(dev, REGISTER(0x607D, 1), minpos); // min_position_limit User specific [-2147483648 qc]
        r &= device_set_register(dev, REGISTER(0x607D, 2), maxpos); // max_position_limit User specific [2147483647 qc]
        r &= device_set_register(dev, REGISTER(0x607F, 0), 8000);   // max_profile_velocity  Motor specific [25000 rpm]
        r &= device_set_register(dev, REGISTER(0x6081, 0),  3000);  // profile_velocity Desired Velocity [1000 rpm]
        r &= device_set_register(dev, REGISTER(0x6083, 0), 10000);  // profile_acceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6084, 0), 10000);  // profile_deceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6085, 0), 10000);  // quickstop_deceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6086, 0),     0);  // normal motion_profile_type: linear ramps  User specific [0]
        // Homing
	r &= device_set_register(dev, REGISTER(0x6098, 0), method); // homing_method           see firmware doc
	r &= device_set_register(dev, REGISTER(0x6099, 1),   1500); // switch_search_speed     User specific [100 rpm] needs to be big because of our gear
        r &= device_set_register(dev, REGISTER(0x6099, 2),    300); // zero_search_speed       User specific [10 rpm]
        r &= device_set_register(dev, REGISTER(0x609A, 0),   5000); // homing_acceleration     User specific [1000 rpm/s]

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

	// homrefed and PPM, now issue SwitchOn
	if (control != CONTROL_SWITCHON) {
                slog(LOG_DEBUG, "rudder_init final switchon");
		device_invalidate_register(dev, REG_CONTROL);
                device_set_register(dev, REG_CONTROL, CONTROL_SWITCHON);
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
        uint32_t opmode;
	int32_t curr_targ_qc;
	int32_t new_targ_qc;

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
        r &= device_get_register(dev, REG_TARGPOS, (uint32_t*)&curr_targ_qc);

        if (!r) return DEFUNCT;

	if (!(status & STATUS_HOMEREF) || opmode != OPMODE_PPM)
		return HOMING;

	if (isnan(target_angle_deg)) return REACHED;

	new_targ_qc = angle_to_qc(params, target_angle_deg);

	if (new_targ_qc != curr_targ_qc) {
                status &= ~STATUS_TARGETREACHED;
                device_invalidate_register(dev, REG_CONTROL);
		device_set_register(dev, REG_TARGPOS, new_targ_qc);
		device_set_register(dev, REG_CONTROL, CONTROL_START);
        }

        device_invalidate_register(dev, REG_STATUS);
        return (status & STATUS_TARGETREACHED) ? REACHED : TARGETTING;
}

// -----------------------------------------------------------------------------

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

	setlinebuf(stdout);

	char label[1024];
	snprintf(label, sizeof label, "%s(%s)", argv0, params->label);
	openlog(label, debug?LOG_PERROR:0, LOG_LOCAL2);
	if(!debug) setlogmask(LOG_UPTO(LOG_NOTICE));

	// ask linebusd to install filters
	printf("$name %s\n", params->label);
	printf("$subscribe 0x%x\n", params->serial_number);
	printf("$subscribe rudderctl:\n");  // must match IFMT_RUDDER_CTL
	fflush(stdout);

	bus = bus_new(stdout);	
	bus_enable_timestamp(bus, dotimestamps);
	dev = bus_open_device(bus, params->serial_number);

	struct Timer reach;
	memset(&reach, 0, sizeof reach);

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


