// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

// Utility to put the boat in 'docking' state: sail and rudders to zero
// Usage: plug /var/run/linebus dock
//
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/time.h>
#include <unistd.h>

#include "../proto/rudder.h"

// static const char* version = "$Id: $";
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

static void fault(int i) { crash("fault"); }

static void
usage(void)
{
        fprintf(stderr,
                "usage: [plug /path/to/linebus] %s\n"
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

	if (argc != 0) usage();

	setlinebuf(stdout);

	if (!debug) openlog(argv0, LOG_PERROR, LOG_LOCAL0);

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");
	
	struct RudderProto ctl = { now_ms(), 0, 0, 0 };
        printf(OFMT_RUDDERPROTO_CTL(ctl));

	struct RudderProto sts = INIT_RUDDERPROTO;

	while (!feof(stdin)) {

		int64_t now = now_ms();
		if (now - ctl.timestamp_ms > 2000) {
			ctl.timestamp_ms = now;
                        printf(OFMT_RUDDERPROTO_CTL(ctl));
		}
			
		if (ferror(stdin)) clearerr(stdin);

		char line[1024];
		if (!fgets(line, sizeof line, stdin)) 
			crash("Reading input");

		if (debug) fprintf(stderr, "Got line:%s\n", line);

                int nn = 0;
                int n = sscanf(line, IFMT_RUDDERPROTO_STS(&sts, &nn));
		if (n != 4) continue;  // not for us

		if (fabs(sts.rudder_l_deg - ctl.rudder_l_deg) < (180.0/256.0)) 
		if (fabs(sts.rudder_r_deg - ctl.rudder_r_deg) < (180.0/256.0)) 
		if (fabs(sts.sail_deg     - ctl.sail_deg)     < (360.0/256.0)) break;

	}

	fprintf(stderr, "Docking done at %lld\n", sts.timestamp_ms);
	exit(0);
}
