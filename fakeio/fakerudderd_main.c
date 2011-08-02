// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Pretend to be the rudderd: read ctl, send plausible sts

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "../proto/rudder.h"

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

static void fault(int i) { crash("fault"); }

static void
usage(void)
{
        fprintf(stderr, "usage: [plug /path/to/linebus] %s\n", argv0);
        exit(1);
}

static void set_fd(fd_set* s, int* maxfd, FILE* f) {
        int fd = fileno(f);
        FD_SET(fd, s);
        if (*maxfd < fd) *maxfd = fd;
}

static int64_t now_ms() 
{
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) crash("no working clock");

        int64_t ms1 = tv.tv_sec;  ms1 *= 1000;
        int64_t ms2 = tv.tv_usec; ms2 /= 1000;
        return ms1 + ms2;
}

static double limit(double v, double min, double max) {
	if (v < min) return min;
	if (v > max) return max;
	return v;
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

	if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) crash("signal(SIGPIPE)");

	// Go daemon and write pidfile.
	if (!debug) {
		daemon(0,1);

		char* path_to_pidfile = NULL;
		asprintf(&path_to_pidfile, "/var/run/%s.pid", argv0);
		FILE* pidfile = fopen(path_to_pidfile, "w");
		if(!pidfile) crash("writing pidfile");
		fprintf(pidfile, "%d\n", getpid());
		fclose(pidfile);
		free(path_to_pidfile);

		syslog(LOG_INFO, "Started.");
	} else {
		fprintf(stderr, "Started.\n");
	}

	int64_t last = now_ms();
	struct RudderProto target = INIT_RUDDERPROTO;
	struct RudderProto actual = { last, 0, 0, 0 };
	
	while  (!feof(stdin)) {

                struct timespec timeout = { 1, 0 }; // wake up at least 1/second
                fd_set rfds;
                fd_set wfds;
		int max_fd = -1;
                FD_ZERO(&rfds);
                FD_ZERO(&wfds);

		set_fd(&rfds, &max_fd, stdin);

                sigset_t empty_mask;
                sigemptyset(&empty_mask);
                int r = pselect(max_fd + 1, &rfds, &wfds, NULL, &timeout, &empty_mask);
                if (r == -1 && errno != EINTR) crash("pselect");

		if (debug>2) fprintf(stderr, "Woke up %d\n", r);

		if (FD_ISSET(fileno(stdin), &rfds)) {
			char line[1024];
			if (fgets(line, sizeof line, stdin)) {
				int nn = 0;
				struct RudderProto newtarget = INIT_RUDDERPROTO;
				int n = sscanf(line, IFMT_RUDDERPROTO_CTL(&newtarget, &nn));
				if (n == 4)
					memmove(&target, &newtarget, sizeof target);
			}
			// fprintf(stderr, "got:%s\n", line);
			if (ferror(stdin)) clearerr(stdin);
		}

		target.rudder_l_deg = limit(target.rudder_l_deg, -90.0, 90.0);
		target.rudder_r_deg = limit(target.rudder_r_deg, -90.0, 90.0);

		int changed = 0;		
		int64_t now = now_ms();

		// rudders move 10 degrees / 1000 milliseconds
		double delta_rudder_deg = (now - actual.timestamp_ms) *  (10.0/1000.0);

		if (target.rudder_l_deg < actual.rudder_l_deg) {
			++changed;
			actual.rudder_l_deg = limit(actual.rudder_l_deg - delta_rudder_deg, target.rudder_l_deg, actual.rudder_l_deg); 
		}
		
		if (target.rudder_l_deg > actual.rudder_l_deg) {
			++changed;
			actual.rudder_l_deg = limit(actual.rudder_l_deg + delta_rudder_deg, actual.rudder_l_deg, target.rudder_l_deg); 
		}

		
		if (target.rudder_r_deg < actual.rudder_r_deg) {
			++changed;
			actual.rudder_r_deg = limit(actual.rudder_r_deg - delta_rudder_deg, target.rudder_r_deg, actual.rudder_r_deg); 
		}
		
		if (target.rudder_r_deg > actual.rudder_r_deg) {
			++changed;
			actual.rudder_r_deg = limit(actual.rudder_r_deg + delta_rudder_deg, actual.rudder_r_deg, target.rudder_r_deg); 
		}

		if (!isnan(target.sail_deg)) {
			double delta_sail_deg = (now - actual.timestamp_ms) *  (18.0/1000.0);
			double sail_diff_deg = target.sail_deg - actual.sail_deg;
			while (sail_diff_deg < -180.0) sail_diff_deg += 360.0;
			while (sail_diff_deg >  180.0) sail_diff_deg -= 360.0;
			if (sail_diff_deg < 0) {
				++changed;
				sail_diff_deg = limit(-delta_sail_deg, sail_diff_deg, 0);
			}
			if (sail_diff_deg > 0) {
				++changed;
				sail_diff_deg = limit(delta_sail_deg, 0, sail_diff_deg);
			}
			actual.sail_deg += sail_diff_deg;
		}

		actual.timestamp_ms = now;
		if (now > last + 1000) ++changed;
		if (now < last + 100) changed = 0;
                if (changed) {
			last = now;
			int nn = 0;
			printf(OFMT_RUDDERPROTO_STS(actual, &nn));
                }

	}  // main loop

        crash("exit loop");

	return 0;
}
