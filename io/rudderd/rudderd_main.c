// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Control rudder and (despite it's name) sail positions.
//
// Open /dev/ttyXXX (with an eposcom slave) or a socket to eposd
// and try to maintain rudder homing bit and target position, and sail
// target position.
//

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "eposclient.h"
#include "rudder.h"

#include "../../proto/rudder.h"

struct MotorParams motor_params[] = {
        { "LEFT",  0x09011145, 100.0, -80.0, 0, -288000 },
        { "RIGHT", 0x09010537, -90.0,  90.0, 0,  288000 },
// sail and bmmh *must* be 360 degree ranges
        { "SAIL",  0x09010506,  -180, 180.0, 615000, -615000 },
	{ "BMMH",  0x00001227,  -180, 180.0, 2048, -2048 }, // 4096 tics for a complete rotation
};

static const char* version = "$Id: $";
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
                "usage: %s /path/to/epos\n"
                , argv0);
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

int main(int argc, char* argv[]) {

	int ch, i;
	char* path_to_eposcom = NULL;

        argv0 = strrchr(argv[0], '/');
        if (argv0) ++argv0; else argv0 = argv[0];

        while ((ch = getopt(argc, argv, "dE:hv")) != -1){
                switch (ch) {
                case 'd': ++debug; break;
		case 'E': path_to_eposcom = optarg; break;
                case 'v': ++verbose; break;
                case 'h':
                default:
                        usage();
                }
        }

	if (!path_to_eposcom) {
		char* c = strrchr(argv[0], '/');
		c = c ? c+1 : argv[0];
		asprintf(&path_to_eposcom, "%.*seposcom", c - argv[0], argv[0]);
	}

	argv += optind;
	argc -= optind;

	if (argc != 1) usage();

	setlinebuf(stdout);

	if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) crash("signal(SIGPIPE)");

	Bus* bus = NULL;
	if (!strncmp("/dev/tty", argv[0], 8)) {
		bus = bus_open_eposcom(path_to_eposcom, argv[0]);
        } else {
		for (i = 0; ; i++) {
			bus = bus_open_eposd(argv[0]);
			if (bus) break;
			fprintf(stderr, "Waiting for %s...%c\r", argv[0], "-\\|/"[i++%4]);
			sleep(1);
		}
                fprintf(stderr, "Waiting for %s...bingo!\n", argv[0]);
        }

	if (!bus) crash("bus open(%s)", argv[0]);

	Device* dev[] = {
		bus_open_device(bus, motor_params[LEFT].serial_number),
		bus_open_device(bus, motor_params[RIGHT].serial_number),
		bus_open_device(bus, motor_params[SAIL].serial_number),
		bus_open_device(bus, motor_params[BMMH].serial_number),
	};

	// todo: actually probe device and close, null on failure.

	for (i = LEFT; i <= BMMH; ++i)
		if (!dev[i]) {
			if (debug) 
				fprintf(stderr, "Error opening %s device (0x%x)\n", motor_params[i].label, motor_params[i].serial_number);
			else
				syslog(LOG_ERR, "Error opening %s device (0x%x)\n", motor_params[i].label, motor_params[i].serial_number);
		}

	if (!(dev[LEFT] || dev[RIGHT] || (dev[SAIL] && dev[BMMH]))) crash("Nothing to control.");

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

	struct RudderProto target = INIT_RUDDERPROTO;
	struct RudderProto actual = INIT_RUDDERPROTO;
	actual.timestamp_ms = now_ms();

	int dev_state[3] = { DEFUNCT, DEFUNCT, DEFUNCT };

	while  (!feof(stdin)) {

                struct timespec timeout = { 1, 0 }; // wake up at least 1/second
                fd_set rfds;
                fd_set wfds;
		int max_fd = -1;
                FD_ZERO(&rfds);
                FD_ZERO(&wfds);

		if (bus_set_fds(bus, &rfds, &wfds, &max_fd))
                        crash("Epos bus closed.\n");

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
			if (ferror(stdin)) clearerr(stdin);
		}

		if (debug>2) fprintf(stderr, "Handling bus\n", r);

 		bus_flush(bus, &wfds);
                bus_receive(bus, &rfds);
		bus_clocktick(bus);  // expire outstanding epos commands

		errno = 0;
                int changed = 0;

		if (dev[LEFT]) {
			double before = actual.rudder_l_deg;
			dev_state[LEFT] = rudder_control(dev[LEFT], &motor_params[LEFT], target.rudder_l_deg, &actual.rudder_l_deg);
			if (before != actual.rudder_l_deg) ++changed;
		}

		if (dev[RIGHT]) {
			double before = actual.rudder_r_deg;
			dev_state[RIGHT] = rudder_control(dev[RIGHT], &motor_params[RIGHT], target.rudder_r_deg, &actual.rudder_r_deg);
			if (before != actual.rudder_r_deg) ++changed;
		}

		if (dev[SAIL] && dev[BMMH]) {
                        double before = actual.sail_deg;
			dev_state[SAIL] = sail_control(dev[SAIL], dev[BMMH],
						       &motor_params[SAIL], &motor_params[BMMH],
						       target.sail_deg, &actual.sail_deg);
                        if (before != actual.sail_deg) ++changed;
                }

		int64_t now = now_ms();

		if (now > actual.timestamp_ms + 1000) ++changed;
		if (now < actual.timestamp_ms + 100) changed = 0;  // rate limit
		if (now < actual.timestamp_ms) ++changed;  // guard against clock jumps

                if (changed) {
			actual.timestamp_ms = now;
			int nn = 0;
			printf(OFMT_RUDDERPROTO_STS(actual, &nn));
                }

	}  // main loop

	bus_close(bus);

        crash("exit loop");

	return 0;
}
