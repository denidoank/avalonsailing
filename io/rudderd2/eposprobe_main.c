// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
//  Output epos probing commands with a fixed frequency. Use plug -o 
//  to connect to the ebus.
//

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "log.h"
#include "actuator.h"

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------
// static const char* version = "$Id: $";
static const char* argv0;
static int verbose = 0;
static int debug = 0;

static void
usage(void)
{
	fprintf(stderr,
		"usage: %s [options] | plug -o /path/to/ebus \n"
		"options:\n"
		"\t-f freq probing frequency [default 8Hz]\n"
		, argv0);
	exit(2);
}

static int64_t
now_us() 
{
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) crash("no working clock");

        int64_t ms1 = tv.tv_sec;  ms1 *= 1000000;
        int64_t ms2 = tv.tv_usec;
        return ms1 + ms2;
}

// -----------------------------------------------------------------------------

int main(int argc, char* argv[]) {

	int ch;
	int freq_hz = 8;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "df:hv")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
		case 'f': freq_hz = atoi(optarg); break;
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

	int64_t delta_us = 1000000/freq_hz;
	int64_t last_us = 0;

	for (;;) {
		int64_t now = now_us();
		if (now < last_us + delta_us) {
			usleep(last_us + delta_us - now);
		}

		printf(EBUS_GET_OFMT(motor_params[LEFT].serial_number, REG_STATUS));
		printf(EBUS_GET_OFMT(motor_params[LEFT].serial_number, REG_CURRPOS));

		printf(EBUS_GET_OFMT(motor_params[RIGHT].serial_number, REG_STATUS));
		printf(EBUS_GET_OFMT(motor_params[RIGHT].serial_number, REG_CURRPOS));

		printf(EBUS_GET_OFMT(motor_params[SAIL].serial_number, REG_STATUS));
		printf(EBUS_GET_OFMT(motor_params[BMMH].serial_number, REG_BMMHPOS));

		fflush(stdout);
		last_us = now_us();
	}

	crash("main loop exit");
	return 0;
}
