// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Listen (on linebus) for imu messages and once a minute call adjtime
// 

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

#include "../proto/imu.h"
#include "log.h"

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
		"\t-g 30 guard time (minutes): if imu time is unavailable for more than this many minutes, exit."
		, argv0);
	exit(2);
}

static uint64_t now_ms() {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0) crash("gettimeofday");
	uint64_t ms1 = tv.tv_sec;  ms1 *= 1000;
	uint64_t ms2 = tv.tv_usec; ms2 /= 1000;
	return ms1 + ms2;
}

static int cmpuint64(const void* a, const void* b) {
	uint64_t *aa = (uint64_t *)a;
	uint64_t *bb = (uint64_t *)b;
	return *aa - *bb;
}

static int alarm_s = 30*60;

void timeout() { crash("No valid imu time for %d minutes.", alarm_s/60); }

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
	char line[1024];
	int nn = 0;
	uint64_t lastadj_ms = 0;
	uint64_t diffs[10];
	const int N = (sizeof(diffs) / sizeof(diffs[0]));
	int i = 0;

	if (alarm_s) alarm(alarm_s);

	while (!feof(stdin)) {
		while(fgets(line, sizeof line, stdin)) {
			if (!sscanf(line, IFMT_IMUPROTO(&imu, &nn))) continue;
			if (imu.timestamp_ms == 0) continue;
			if (alarm_s) alarm(alarm_s);
			if (imu.timestamp_ms - lastadj_ms < 60*1000) continue;
			int64_t now = now_ms();
			int ii = ++i % N;
			diffs[ii] = imu.timestamp_ms - now;
			if (ii) continue;
			// got N samples.  take average minus 2-outliers
			qsort(diffs, N, sizeof diffs[0], cmpuint64);
			int j;
			int64_t avg = 0;
			for (j = 2; j < N-2; ++j) avg += diffs[j];
			avg /= N-4;
			if ((avg > -1000) && (avg < 1000)) continue;   // don't bother if less than a second
			lastadj_ms = imu.timestamp_ms;
			if ((avg > -1000*1000) && (avg < 1000*1000)) {
				syslog(LOG_INFO, "Adjusting time by %lldms", avg);
				struct timeval delta = { avg/1000, (avg % 1000)*1000 };
				if (adjtime(&delta, NULL))
					if (!debug) crash("adjtime");
				continue;
			}

			syslog(LOG_INFO, "Setting system time (%+lldms).", imu.timestamp_ms - now);
			struct timeval newnow = { imu.timestamp_ms/1000, 0 };
			if (settimeofday(&newnow, NULL))
				if (!debug) crash("settimeofday");
			syslog(LOG_INFO, "Set system time (%+lldms).", imu.timestamp_ms - now);
		}
		if (ferror(stdin)) clearerr(stdin);
	}

	crash("Exit loop");
	return 0;
}

 
