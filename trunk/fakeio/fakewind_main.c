// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

// Pretend to be the wind daemon
// Usage: plug /var/run/linebus fakewind direction strength
// Does not compensate for the boats speed!

#include <errno.h>
#include <math.h>
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

#include "../proto/rudder.h"
#include "../proto/imu.h"
#include "../proto/wind.h"

//static const char* version = "$Id: $";
static const char* argv0;
static int verbose = 0;
static int debug = 0;

static void
crash(const char* fmt, ...)
{
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

static void fault(int dum) { crash("fault"); }

static void
usage(void)
{
        fprintf(stderr,
                "usage: [plug /path/to/linebus] %s [direction_deg [strength_ms]]\n"
                , argv0);
        exit(1);
}

static int64_t now_ms() 
{
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) crash("no working clock");

        int64_t ms1 = tv.tv_sec;  ms1 *= 1000;
        int64_t ms2 = tv.tv_usec; ms2 /= 1000;
        return ms1 + ms2;
}

int main(int argc, char* argv[]) {

	int ch;

	double angle_deg = 245.0;
	double speed_m_s = 5.0;

        argv0 = strrchr(argv[0], '/');
        if (argv0) ++argv0; else argv0 = argv[0];

        while ((ch = getopt(argc, argv, "dhv")) != -1){
                switch (ch) {
                case 'd': ++debug; break;
                case 'v': ++verbose; break;
                case 'h':
                default:
                        usage();
                }
        }

	argv += optind;
	argc -= optind;

	if (argc > 0) {
		angle_deg = atof(argv[0]);
		argc--; argv++;
	}

	if (argc > 0) {
		speed_m_s = atof(argv[0]);
		argc--; argv++;
	}
	if (argc != 0) usage();

	setlinebuf(stdout);

	if (!debug) openlog(argv0, LOG_PERROR, LOG_LOCAL0);

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");

	int nn = 0;

	int64_t last = now_ms();
	struct RudderProto rudd = INIT_RUDDERPROTO;
	struct IMUProto    imud = INIT_IMUPROTO;

	while (!feof(stdin)) {
			
		if (ferror(stdin)) clearerr(stdin);

		char line[1024];
		if (!fgets(line, sizeof line, stdin)) 
			crash("Reading input");

		int n1 = sscanf(line, IFMT_RUDDERPROTO_STS(&rudd, &nn));
		int n2 = sscanf(line, IFMT_IMUPROTO(&imud, &nn));

		if (debug) fprintf(stderr, "Got line:%s  rudd:%d imu:%d\n", line, n1, n2);

		int64_t now = now_ms();
		if (now < last + 1000) continue;
		last = now;

		struct WindProto wind = { now, angle_deg - rudd.sail_deg - imud.yaw_deg, -1, speed_m_s, 1 };
                printf(OFMT_WINDPROTO(wind));
	}

	crash("Terminating.");

	return 0;
}
