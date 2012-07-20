// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Listen (on linebus) for imu/gps messages and once a minute call adjtime
//
// TODO: report difference between gps and imu?

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/time.h>
#include <unistd.h>

#include "proto/imu.h"
#include "proto/gps.h"
#include "lib/log.h"

// -----------------------------------------------------------------------------
// static const char* version = "$Id: $";
static const char* argv0;
static int debug = 0;

static void
usage(void)
{
	fprintf(stderr,
		"usage: [plug -o /path/to/bus |] %s [options]\n"
		"options:\n"
		"\t-d debug (don't syslog)\n"
		"\t-g 30 guard time (minutes): if imu/gps time is unavailable for more than this many minutes, exit. 0 to disable."
		, argv0);
	exit(2);
}

static int cmpint64(const void* a, const void* b) {
	int64_t *aa = (int64_t *)a;
	int64_t *bb = (int64_t *)b;
	return *aa - *bb;
}

static int alarm_s = 30*60;

static void timeout() { crash("No valid imu/gps time for %d minutes.", alarm_s/60); }

int main(int argc, char* argv[]) {
	int ch;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "dg:h")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
		case 'g': alarm_s = 60*atoi(optarg); break;
		case 'h': 
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 0) usage();

	openlog(argv0, debug?LOG_PERROR:0, LOG_DAEMON);

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");
	if (signal(SIGALRM, timeout) == SIG_ERR)  crash("signal(SIGSALRM)");

	struct IMUProto imu = INIT_IMUPROTO;
	struct GPSProto gps = INIT_GPSPROTO;
	int usegps = 0;
	char line[1024];
	int nn = 0;
	uint64_t lastadj_ms = 0;
	uint64_t diffs[10];
	const int N = (sizeof(diffs) / sizeof(diffs[0]));
	int i = 0;

	if (alarm_s) alarm(alarm_s);

	while (!feof(stdin)) {

		while(fgets(line, sizeof line, stdin)) {

			int64_t sys_timestamp_ms;
			int64_t gps_timestamp_ms;

			if (sscanf(line, IFMT_IMUPROTO(&imu, &nn)) >= 2 && imu.gps_timestamp_ms != 0) {

				if (alarm_s) alarm(alarm_s);

				sys_timestamp_ms = imu.timestamp_ms;
				gps_timestamp_ms = imu.gps_timestamp_ms;

				if (usegps) syslog(LOG_NOTICE, "imu time source is back.");
				usegps = 0;

			} else if (sscanf(line, IFMT_GPSPROTO(&gps, &nn)) >= 2 && gps.gps_timestamp_ms != 0) {

				if (alarm_s) alarm(alarm_s);

				// if we have seen an "imu: ..." in the last minute, don't use "gps: ..."
				if (imu.gps_timestamp_ms + 60*1000 > gps.gps_timestamp_ms )
					continue;

				if (!usegps) syslog(LOG_NOTICE, "imu time source gone for more than a minute, falling back to gps.");
				usegps = 1;

				sys_timestamp_ms = gps.timestamp_ms;
				gps_timestamp_ms = gps.gps_timestamp_ms;

			} else continue;

			if (gps_timestamp_ms - lastadj_ms < 60*1000) continue;

			int ii = ++i % N;
			diffs[ii] = gps_timestamp_ms - sys_timestamp_ms;
			if (ii) continue;
			// got N samples.  take average minus 2-outliers
			qsort(diffs, N, sizeof diffs[0], cmpint64);
			int j;
			int64_t avg = 0;
			for (j = 2; j < N-2; ++j) avg += diffs[j];
			avg /= N-4;
#if 0
			if ((avg > -1000) && (avg < 1000)) continue;   // don't bother if less than a second
#endif

			lastadj_ms = gps_timestamp_ms;
			if ((avg > -1000*1000) && (avg < 1000*1000)) {

				syslog(LOG_INFO, "Adjusting time by %lldms", avg);
				struct timeval delta = { avg/1000, (avg % 1000)*1000 };
				if (adjtime(&delta, NULL))
					if (!debug) crash("adjtime");

			} else {

				syslog(LOG_INFO, "Setting system time (%+lldms).", gps_timestamp_ms - sys_timestamp_ms);
				struct timeval newnow = { gps_timestamp_ms/1000, (gps_timestamp_ms % 1000)*1000 };
				if (settimeofday(&newnow, NULL))
					if (!debug) crash("settimeofday");

			}
		}

		if (ferror(stdin)) clearerr(stdin);
	}

	crash("Exit loop");
	return 0;
}

 
