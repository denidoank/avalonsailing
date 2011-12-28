// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Issue epos commands to keep one rudder (-l[eft] or -r[ight])
// homed and close to the reference value
// 

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "../../proto/rudder.h"

#include "actuator.h"
#include "eposclient.h"

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------
// static const char* version = "$Id: $";
static const char* argv0;
static int verbose = 0;
static int debug = 0;


static void crash(const char* fmt, ...) {
	va_list ap;
	char buf[1000];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	if (debug)
		fprintf(stderr, "%s:%s%s%s\n", argv0, buf,
			(errno) ? ": " : "",
			(errno) ? strerror(errno):"" );
	else
		syslog(LOG_CRIT, "%s%s%s\n", buf,
		       (errno) ? ": " : "",
		       (errno) ? strerror(errno):"" );
	exit(1);
	va_end(ap);
	return;
}

static void fault() { crash("fault"); }

static void usage(void) {
	fprintf(stderr,	"usage: plug /path/to/ebus | %s {-l | -r} \n", argv0);
	exit(2);
}

static int64_t now_ms() {
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) crash("no working clock");

        int64_t ms1 = tv.tv_sec;  ms1 *= 1000;
        int64_t ms2 = tv.tv_usec; ms2 /= 1000;
        return ms1 + ms2;
}

static Bus* bus = NULL;
static MotorParams* params = NULL;
static Device* dev = NULL;
static double target_angle_deg = NAN;

// aim for about 12 bits of resolution: 180/4096 ~ .043
const double TOLERANCE_DEG = .04;  // aiming precision in targetting rudder

static int processinput() {
	char line[1024];
	if (!fgets(line, sizeof(line), stdin))
		crash("reading stdin");

	if (line[0] == '#') {
		struct RudderProto msg = INIT_RUDDERPROTO;
		int nn;
		int n = sscanf(line+1, IFMT_RUDDERPROTO_CTL(&msg, &nn));
		if (n != 4)
			return 0;
		target_angle_deg = (params == &motor_params[LEFT]) ? msg.rudder_l_deg : msg.rudder_r_deg;
	} else {
		if (!bus_receive(bus, line))
			return 0;
		bus_clocktick(bus);
	}
	return 1;
}

enum { DEFUNCT = 0, HOMING, TARGETTING, REACHED };
const char* status[] = { "DEFUNCT", "HOMING", "TARGETTING", "REACHED" };

// todo, use -d flag and fprintf
#define VLOGF(...) syslog(LOG_DEBUG, __VA_ARGS__)

// Work towards a state where the homed bit is set and we're in PPM mode
static int rudder_init()
{
        int r;
        uint32_t status;
        uint32_t error;
        uint32_t control;
        uint32_t opmode;

        VLOGF("rudder_init");

        if (!device_get_register(dev, REG_STATUS, &status))
                return DEFUNCT;

        if (status & STATUS_FAULT) {
		VLOGF("rudder_init clearing fault");
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
        minpos -= 10*delta;
        maxpos += 10*delta;
        int32_t method = (params->home_pos_qc < params->extr_pos_qc) ? 1 : 2;

        r &= device_set_register(dev, REGISTER(0x2080, 0), 3000);   // current_threshold       User specific [500 mA]
        r &= device_set_register(dev, REGISTER(0x2081, 0), 0); 	    // home_position User specific [0 qc]
        r &= device_set_register(dev, REGISTER(0x6065, 0), 50*delta); // max_following_error User specific [2000 qc]
        r &= device_set_register(dev, REGISTER(0x6067, 0), delta);  // position window [qc], see 14.66
        r &= device_set_register(dev, REGISTER(0x6068, 0), 50);     // position time window [ms], see 14.66
        r &= device_set_register(dev, REGISTER(0x607C, 0), 0);      // home_offset User specific [0 qc]
        r &= device_set_register(dev, REGISTER(0x607D, 1), minpos); // min_position_limit User specific [-2147483648 qc]
        r &= device_set_register(dev, REGISTER(0x607D, 2), maxpos); // max_position_limit User specific [2147483647 qc]
        r &= device_set_register(dev, REGISTER(0x607F, 0), 15000);  // max_profile_velocity  Motor specific [25000 rpm]
        r &= device_set_register(dev, REGISTER(0x6081, 0),  5000);  // profile_velocity Desired Velocity [1000 rpm]
        r &= device_set_register(dev, REGISTER(0x6083, 0), 10000);  // profile_acceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6084, 0), 10000);  // profile_deceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6085, 0), 10000);  // quickstop_deceleration User specific [10000 rpm/s]
        r &= device_set_register(dev, REGISTER(0x6086, 0), 0);      // motion_profile_type  User specific [0]
        r &= device_set_register(dev, REGISTER(0x6098, 0), method); // homing_method           see firmware doc
        r &= device_set_register(dev, REGISTER(0x6099, 1),   200);  // switch_search_speed     User specific [100 rpm]
        r &= device_set_register(dev, REGISTER(0x6099, 2),    10);  // zero_search_speed       User specific [10 rpm]
        r &= device_set_register(dev, REGISTER(0x609A, 0), 10000);  // homing_acceleration     User specific [1000 rpm/s]

        if (!r) {
                device_invalidate_register(dev, REG_CONTROL);
                device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                return DEFUNCT;
        }
        VLOGF("rudder_init configured");

        if (!(status & STATUS_HOMEREF)) {
                if (opmode != OPMODE_HOMING) {
                        VLOGF("rudder_init set opmode homing");
                        device_set_register(dev, REG_OPMODE, OPMODE_HOMING);
                        device_invalidate_register(dev, REG_CONTROL);
                        device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                        device_invalidate_register(dev, REG_STATUS);
                        return HOMING;
                }

                switch (control) {
                case CONTROL_SHUTDOWN:
                        VLOGF("rudder_init homing, switchon");
                        device_set_register(dev, REG_CONTROL, CONTROL_SWITCHON);
                        break;

                case CONTROL_SWITCHON:
                        VLOGF("rudder_init homing, start");
                        device_set_register(dev, REG_CONTROL, CONTROL_START);
                        break;

                case CONTROL_START:
                        VLOGF("rudder_init homing, waiting");
                        if (!(status & STATUS_HOMINGERROR))
                                break;
                        VLOGF("rudder_init homing error: %x", status);
                        // fallthrough

                default:
                        VLOGF("rudder_init homing bad state: control %x, status %x", control, status);
                        device_invalidate_register(dev, REG_OPMODE);
                        device_set_register(dev, REG_OPMODE, OPMODE_HOMING);
                        device_invalidate_register(dev, REG_CONTROL);
                        device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                }

                device_invalidate_register(dev, REG_STATUS);
                return HOMING;

        } // not HOMEREF'ed

        VLOGF("rudder_init homeref ok.");

        if (opmode != OPMODE_PPM) {
                VLOGF("rudder_init set opmode PPM");
                device_set_register(dev, REG_OPMODE, OPMODE_PPM);
                device_invalidate_register(dev, REG_CONTROL);
                device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
                device_invalidate_register(dev, REG_STATUS);
		return DEFUNCT;
        }

	return TARGETTING;
}

static int rudder_control(double* actual_angle_deg)
{
        int r;
        uint32_t status;
        uint32_t error;
        uint32_t control;
        uint32_t opmode;
        int32_t curr_pos_qc;
        int32_t curr_targ_qc;

        VLOGF("rudder_control");

        if (!device_get_register(dev, REG_STATUS, &status))
                return DEFUNCT;

        if (status & STATUS_FAULT) {
                VLOGF("rudder_control: clearing fault");
                device_invalidate_register(dev, REG_CONTROL);   // force re-issue
                device_set_register(dev, REG_CONTROL, CONTROL_CLEARFAULT);
                device_invalidate_register(dev, REG_ERROR);   // force re-issue
		device_get_register(dev, REG_ERROR, &error);  // we won't read it but eposmon will pick up the response.
                device_invalidate_register(dev, REG_STATUS);
                return DEFUNCT;
        }

	if (!(status & STATUS_HOMEREF))
		return HOMING;

        r  = device_get_register(dev, REG_CONTROL, &control);
        r &= device_get_register(dev, REG_OPMODE,  &opmode);
        r &= device_get_register(dev, REG_CURRPOS, (uint32_t*)&curr_pos_qc);
        r &= device_get_register(dev, REG_TARGPOS, (uint32_t*)&curr_targ_qc);

        if (!r) return DEFUNCT;

        *actual_angle_deg = qc_to_angle(params, curr_pos_qc);

	if (opmode != OPMODE_PPM)
		return HOMING;

        VLOGF("rudder_control current position %x(%d %f) current target %x(%d %f)",
              curr_pos_qc,  curr_pos_qc,  qc_to_angle(params, curr_pos_qc),
              curr_targ_qc, curr_targ_qc, qc_to_angle(params, curr_targ_qc));


	if (isnan(target_angle_deg)) return REACHED;

        // no matter where we are, if current target is away from new target angle, we're not there

        double targ_diff = target_angle_deg - qc_to_angle(params, curr_targ_qc);
     
        if (targ_diff < 0) targ_diff = -targ_diff;
        if (targ_diff > TOLERANCE_DEG) {
                VLOGF("rudder_control target reached, but wrong target");
                status &= ~STATUS_TARGETREACHED;
                control &= ~0x30;  // if we started, pretend we're just switched on
                device_invalidate_register(dev, REG_CONTROL);
        }

        // targetreached is set, but are we really there?
        if (status & STATUS_TARGETREACHED) {
                double actual_diff = qc_to_angle(params, curr_pos_qc) - qc_to_angle(params, curr_targ_qc);
                if (actual_diff < 0) actual_diff = -actual_diff;
                if (actual_diff > TOLERANCE_DEG) {
                        VLOGF("rudder_control target reached, but actual is wrong");
                        status &= ~STATUS_TARGETREACHED;
                }
        }

        if (!(status & STATUS_TARGETREACHED)) {
                switch (control) {
                case CONTROL_SHUTDOWN:
                        VLOGF("rudder_control targetting, switchon");
                        device_set_register(dev, REG_CONTROL, CONTROL_SWITCHON);
                        break;

                case CONTROL_SWITCHON:
                        VLOGF("rudder_control setting target position %f -> %f", qc_to_angle(params, curr_targ_qc), target_angle_deg);
                        device_invalidate_register(dev, REG_TARGPOS);
                        device_set_register(dev, REG_TARGPOS, angle_to_qc(params, target_angle_deg));
                        device_set_register(dev, REG_CONTROL, CONTROL_START);
                        device_invalidate_register(dev, REG_CURRPOS);
                        break;

                case CONTROL_START:
                case CONTROL_SWITCHON | 0x0010:
                        VLOGF("rudder_control targetting, waiting");
                        if (!(status & STATUS_HOMINGERROR))
                                break;
                        // fallthrough
                default:
                        VLOGF("rudder_control targetting bad state: control %x, status %x", control, status);
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

        VLOGF("rudder_control target reached, shutdown");
        device_set_register(dev, REG_CONTROL, CONTROL_SHUTDOWN);
        // we're homeref, position is close enough, and we powered off

        device_invalidate_register(dev, REG_CURRPOS);
        device_invalidate_register(dev, REG_STATUS);
        return REACHED;
}

// -----------------------------------------------------------------------------

const int64_t WARN_INTERVAL = 5000;
const int64_t MAX_INTERVAL = 15*60*1000;

int main(int argc, char* argv[]) {

	int ch;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "dhlrv")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
		case 'l': params = &motor_params[LEFT]; break;
		case 'r': params = &motor_params[RIGHT]; break;
		case 'v': ++verbose; break;
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

	if (!debug) openlog(label, LOG_PERROR, LOG_DAEMON);

	setlinebuf(stdout);
	bus = bus_new(stdout);	
	dev = bus_open_device(bus, params->serial_number);

	int64_t last_reached = now_ms();
	int64_t warn_interval = WARN_INTERVAL;
	double angle_deg = NAN;
	int state = DEFUNCT;

	for (;;) {
		syslog(LOG_WARNING, "Initializing rudder.");

		device_invalidate_all(dev);

		while (state != TARGETTING)
			if (processinput())
				state = rudder_init();

		syslog(LOG_WARNING, "Done initalizing rudder.");

		while (state != HOMING) {
			if (processinput())
				state = rudder_control(&angle_deg);

			if (isnan(target_angle_deg))
			    continue;

			int64_t now = now_ms();
			
			if (state == REACHED  || (now < last_reached)) {
				last_reached = now;
				warn_interval = WARN_INTERVAL;
			}

			if (now - last_reached > warn_interval) {
				syslog(LOG_WARNING, "Unable to target rudder.");
				last_reached = now;  // shut up warning
				if (warn_interval < MAX_INTERVAL) warn_interval *= 2;
			}
		}
	}

	crash("main loop exit");
	return 0;
}


