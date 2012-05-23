// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Decode status registers on ebus and output drive status messages on lbus.
// Use plug -o and plug -i to connect to the busses.
// 

#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "../../proto/rudder.h"
#include "../timer.h"
#include "../log.h"
#include "actuator.h"

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------
// static const char* version = "$Id: $";
static const char* argv0;
static int verbose = 0;
static int debug = 0;

static void usage(void) {
	fprintf(stderr,
		"usage: plug -i /path/to/ebus | %s [options] | plug -o /path/to/lbus \n"
		"options:\n"
		"\t-n miNimum time [ms] between reports (default 250ms)\n"
		"\t-x maXimum time [ms] between reports (default 1s)\n"
		, argv0);
	exit(2);
}

int main(int argc, char* argv[]) {

	int ch;
	int64_t min_us = 125 *1000;
	int64_t max_us = 1000 *1000;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "dhn:vx:")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
		case 'n': min_us = 1000*atoi(optarg); break;
		case 'x': max_us = 1000*atoi(optarg); break;
		case 'v': ++verbose; break;
		case 'h': 
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 0) usage();

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");

	openlog(argv0, debug?LOG_PERROR:0, LOG_LOCAL2);

	setlinebuf(stdout);

	struct RudderProto sts = INIT_RUDDERPROTO;
	int homed[2] = { 0, 0 };

	struct Timer timer[3];
	memset(&timer, 0, sizeof timer);

	uint64_t us 	= now_us();
	timer_tick(&timer[LEFT], us, 1);
	timer_tick(&timer[RIGHT], us, 1);
	timer_tick(&timer[SAIL], us, 1);

	while(!feof(stdin)) {
		char line[1024];
		if (!fgets(line, sizeof(line), stdin))
			crash("reading stdin");
		
		uint32_t serial = 0;
		uint32_t reg    = 0;
		char op 	= 0;
		int32_t value 	= 0;
		us = 0;

		if (!ebus_parse_rsp(line, &op, &serial, &reg, &value, &us))
			continue;

		if (us == 0) us = now_us();

		if (serial == motor_params[BMMH].serial_number && reg == REG_BMMHPOS) {
			sts.timestamp_ms = us * 1000;
			if (value >= (1<<29)) value -= (1<<30); // bmmh is 30 bit signed
			value &= 4095;
			sts.sail_deg = qc_to_angle(&motor_params[BMMH], value);
			while (sts.sail_deg < -180.0) sts.sail_deg += 360.0;
			while (sts.sail_deg >  180.0) sts.sail_deg -= 360.0;

			if (us > timer_started(&timer[SAIL]) + min_us) {
				timer_tick(&timer[SAIL], us, 0);
				printf(OFMT_STATUS_SAIL(sts));
				timer_tick(&timer[SAIL], us, 1);
			}

		} else if (serial == motor_params[LEFT].serial_number && reg == REG_STATUS) {

			homed[LEFT] = value&STATUS_HOMEREF;

		} else if (serial == motor_params[RIGHT].serial_number && reg == REG_STATUS) {

			homed[RIGHT] = value&STATUS_HOMEREF;

		} else if (serial == motor_params[LEFT].serial_number && reg == REG_CURRPOS && homed[LEFT]) {
			sts.timestamp_ms = us * 1000;
			sts.rudder_l_deg = qc_to_angle(&motor_params[LEFT], value);
			if (homed[LEFT] && (us > timer_started(&timer[LEFT]) + min_us)) {
				timer_tick(&timer[LEFT], us, 0);
				printf(OFMT_STATUS_LEFT(sts));
				timer_tick(&timer[LEFT], us, 1);
			}

		} else if (serial == motor_params[RIGHT].serial_number && reg == REG_CURRPOS && homed[RIGHT]) {
			sts.timestamp_ms = us * 1000;
			sts.rudder_r_deg = qc_to_angle(&motor_params[RIGHT], value);
			if (homed[RIGHT] && (us > timer_started(&timer[RIGHT]) + min_us )) {
				timer_tick(&timer[RIGHT], us, 0);
				printf(OFMT_STATUS_RIGHT(sts));
				timer_tick(&timer[RIGHT], us, 1);
			}
		}

		// if it's been too long, send anyway

		if (homed[LEFT] && (us > timer_started(&timer[LEFT]) + max_us)) {
			timer_tick(&timer[LEFT], us, 0);
			printf(OFMT_STATUS_LEFT(sts));
			timer_tick(&timer[LEFT], us, 1);
		}

		if (homed[RIGHT] && (us > timer_started(&timer[RIGHT]) + max_us )) {
			timer_tick(&timer[RIGHT], us, 0);
			printf(OFMT_STATUS_RIGHT(sts));
			timer_tick(&timer[RIGHT], us, 1);
		}

		if (us > timer_started(&timer[SAIL]) + min_us) {
			timer_tick(&timer[SAIL], us, 0);
			printf(OFMT_STATUS_SAIL(sts));
			timer_tick(&timer[SAIL], us, 1);
		}
	}

	crash("main loop exit");
	return 0;
}
