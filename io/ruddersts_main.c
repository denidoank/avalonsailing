// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Decode status registers on ebus and output ruddersts messages on lbus.
// Use plug -o and plug -i to connect to the busses.
// 

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "proto/rudder.h"
#include "log.h"
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

static int64_t now_us() {
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) crash("no working clock");

        int64_t us1 = tv.tv_sec;  us1 *= 1000000;
        int64_t us2 = tv.tv_usec;
        return us1 + us2;
}

// update x and return true if abs(difference) > .1
static int upd(double* x, double y) {
	double r = y - *x;
	*x = y;
	return r*r > .01;
}

// -----------------------------------------------------------------------------

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

	if(setvbuf(stdout, NULL, _IOLBF, 0))
		syslog(LOG_WARNING, "Failed to make stdout line-buffered.");

	struct RudderProto sts = INIT_RUDDERPROTO;

	int64_t last_us = now_us();

	int homed[2] = { 0, 0 };

	while(!feof(stdin)) {
		char line[1024];
		if (!fgets(line, sizeof(line), stdin))
			crash("reading stdin");

		 if (line[0] == '#') continue;

		int64_t now = now_us();

                uint32_t serial = 0;
		int index       = 0;
		int subindex    = 0;
		int64_t value_l = 0;  // fumble signedness

		int n = sscanf(line, "%i:%i[%i] = %lli", &serial, &index, &subindex, &value_l);
		if (n != 4) continue;
		
		uint32_t value = value_l;
		int reg = REGISTER(index,subindex);
		int changed = 0;

		if (serial == motor_params[BMMH].serial_number && reg == REG_BMMHPOS) {
			if (value >= (1<<29)) value -= (1<<30); // bmmh pos is 30 bit signed
			value &= 4095;
			changed = upd(&sts.sail_deg, qc_to_angle(&motor_params[BMMH], value));
			while (sts.sail_deg < -180.0) sts.sail_deg += 360.0;
			while (sts.sail_deg >  180.0) sts.sail_deg -= 360.0;

		} else if (serial == motor_params[LEFT].serial_number && reg == REG_STATUS) {
			homed[LEFT] = value&STATUS_HOMEREF;
			changed = (!homed[LEFT] && !isnan(sts.rudder_l_deg));
			if (changed) sts.rudder_l_deg = NAN;
		} else if (serial == motor_params[RIGHT].serial_number && reg == REG_STATUS) {
			homed[RIGHT] = value&STATUS_HOMEREF;
			changed = (!homed[RIGHT] && !isnan(sts.rudder_r_deg));
			if (changed) sts.rudder_r_deg = NAN;
		} else if (serial == motor_params[LEFT].serial_number && reg == REG_CURRPOS && homed[LEFT]) {
			changed = upd(&sts.rudder_l_deg, qc_to_angle(&motor_params[LEFT], value));
		} else if (serial == motor_params[RIGHT].serial_number && reg == REG_CURRPOS && homed[RIGHT]) {
			changed = upd(&sts.rudder_r_deg, qc_to_angle(&motor_params[RIGHT], value));
		}

		if (!changed && (last_us + max_us > now)) {
			continue;
		}
		
		sts.timestamp_ms = now / 1000;
	       
		if (last_us + min_us > now)
			continue;

		printf(OFMT_RUDDERPROTO_STS(sts));
		last_us = now_us();
	}

	crash("main loop exit");
	return 0;
}
