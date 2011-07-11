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

struct MotorParams motor_params[] = {
        { "LEFT",  0x09011145,  90.0, -90.0, 0, -288000 },
        { "RIGHT", 0x09010537, -90.0,  90.0, 0,  288000 },
// sail and bmmh *must* be 360 degree ranges
        { "SAIL",  0x09010506,  -180, 180.0, -615000, 615000 },
	{ "BMMH",  0x00001227,  -180, 180.0, -2048, 2048 }, // 4096 tics for a complete rotation
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

static void
usage(void)
{
        fprintf(stderr,
                "usage: %s /path/to/tty_or_socket\n"
                , argv0);
        exit(1);
}

static const char* varnames[] = {
	"rudder_l_deg",
	"rudder_r_deg",
	"sail_deg",
};

static int
parseline(char* line, double* angles_deg)
{
        while (*line) {
                char key[16];
                double value;
                int skip = 0;
                int n = sscanf(line, "%16[a-z_]:%lf %n", key, &value, &skip);
                if (n < 2) return 0;
                if (skip == 0) return 0;
                line += skip;

		if (!strcmp(key, varnames[LEFT]))  { angles_deg[LEFT]  = value; continue; }
		if (!strcmp(key, varnames[RIGHT])) { angles_deg[RIGHT] = value; continue; }
		if (!strcmp(key, varnames[SAIL]))  { angles_deg[SAIL]  = value; continue; }
#if 1           // nice for manual control, disable in prod
		if (!strcmp(key, "rudd"))  { angles_deg[LEFT] = angles_deg[RIGHT] = value; continue; }
		if (!strcmp(key, "sail"))  { angles_deg[SAIL] = value; continue; }
#endif
                if (!strcmp(key, "timestamp_ms"))  continue;
                return 0;
        }
        return 1;
}

static void set_fd(fd_set* s, int* maxfd, FILE* f) {
        int fd = fileno(f);
        FD_SET(fd, s);
        if (*maxfd < fd) *maxfd = fd;
}

struct Client {
        struct Client* next;
        FILE* in;
        FILE* out;
        struct sockaddr_un addr;
        int addrlen;
        int pending;  // possibly unflushed data in out buffer
} *clients = NULL;

static struct Client*
new_client(int sck)
{
        struct Client* cl = malloc(sizeof *cl);
        bzero(cl, sizeof(*cl));
        cl->next = clients;
        cl->addrlen = sizeof(cl->addr);
        int fd = accept(sck, (struct sockaddr*)&cl->addr, &cl->addrlen);
        if (fd < 0) {
		if (debug) fprintf(stderr, "accept:%s\n", strerror(errno));
                free(cl);
                return NULL;
        }
        cl->in  = fdopen(fd, "r");
        cl->out = fdopen(dup(fd), "w");
        if (fcntl(fileno(cl->in),  F_SETFL, O_NONBLOCK) < 0) crash("fcntl(in)");
        if (fcntl(fileno(cl->out), F_SETFL, O_NONBLOCK) < 0) crash("fcntl(out)");
        setbuffer(cl->out, NULL, 64<<10); // 64k out buffer
        cl->pending = 0;
        clients = cl;
        if (debug) fprintf(stderr, "new client %d\n", fd);
        return cl;
}

// write line to client.
static void client_puts(struct Client* client, char* line) {
        if (!client->out) return;
        client->pending = 1;
        if (fputs(line, client->out) >= 0) return;
        if (feof(client->out)) {
                client->pending = 0;
                fclose(client->out);
                client->out = NULL;
        }
}

static void client_flush(struct Client* client) {
        if (!client->out) return;
        if (fflush(client->out) == 0)
                client->pending = 0;
        if (feof(client->out)) {
                client->pending = 0;
                fclose(client->out);
                client->out = 0;
        }
}

int main(int argc, char* argv[]) {

	int ch, i;
	char* path_to_eposcom = NULL;
	char* path_to_socket = NULL;

        argv0 = strrchr(argv[0], '/');
        if (argv0) ++argv0; else argv0 = argv[0];

        while ((ch = getopt(argc, argv, "dE:hs:v")) != -1){
                switch (ch) {
                case 'd': ++debug; break;
		case 'E': path_to_eposcom = optarg; break;
		case 's': path_to_socket = optarg; break;
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

	if (!path_to_socket) {
		char* c = strrchr(argv[0], '/');
		c = c ? c+1 : argv[0];
		asprintf(&path_to_socket, "/var/run/%s", c);
	}

	argv += optind;
	argc -= optind;

	if (argc != 1) usage();

	if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) crash("signal");

        // Set up socket
	unlink(path_to_socket);

	int sck = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sck < 0) crash("socket");
	struct sockaddr_un srvaddr = { AF_LOCAL, 0 };
	strncpy(srvaddr.sun_path, path_to_socket, sizeof(srvaddr.sun_path));
	if (bind(sck, (struct sockaddr*) &srvaddr, sizeof(srvaddr)) < 0)
                crash("bind");
	if (listen(sck, 8) < 0) crash("listen");

        if (debug) fprintf(stderr, "created socket:%s\n", path_to_socket);

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
		daemon(0,0);

		char* path_to_pidfile = NULL;
		asprintf(&path_to_pidfile, "%s.pid", path_to_socket);
		FILE* pidfile = fopen(path_to_pidfile, "w");
		if(!pidfile) crash("writing pidfile");
		fprintf(pidfile, "%d\n", getpid());
		fclose(pidfile);
		free(path_to_pidfile);

		syslog(LOG_INFO, "Started.");
	}

	double target_angles_deg[3] = { NAN, NAN, NAN };
	double actual_angles_deg[3] = { NAN, NAN, NAN };
	int dev_state[3] = { DEFUNCT, DEFUNCT, DEFUNCT };

	for (;;) {

                struct timespec timeout = { 1, 0 }; // wake up at least 1/second
                fd_set rfds;
                fd_set wfds;
		int max_fd = -1;
                FD_ZERO(&rfds);
                FD_ZERO(&wfds);

		FD_SET(sck, &rfds);
		max_fd = sck;

		if (bus_set_fds(bus, &rfds, &wfds, &max_fd))
                        crash("Epos bus closed.\n");

                struct Client* cl;
                for (cl = clients; cl; cl = cl->next)
                        if (cl->out && cl->pending)
                                set_fd(&wfds, &max_fd, cl->out);

                for (cl = clients; cl; cl = cl->next)
                        if (cl->in)
                                set_fd(&rfds, &max_fd, cl->in);

                sigset_t empty_mask;
                sigemptyset(&empty_mask);
                int r = pselect(max_fd + 1, &rfds, &wfds, NULL, &timeout, &empty_mask);
                if (r == -1 && errno != EINTR) crash("pselect");

                if (FD_ISSET(sck, &rfds)) new_client(sck);

                for (cl = clients; cl; cl = cl->next)
                        if (cl->out && FD_ISSET(fileno(cl->out), &wfds))
                                client_flush(cl);

                for (cl = clients; cl; cl = cl->next) {
                        if (!cl->in) continue;
                        if (cl->in && FD_ISSET(fileno(cl->in), &rfds)) {
                                char line[1024];
                                while(fgets(line, sizeof line, cl->in)) {
                                        parseline(line, target_angles_deg);
                                }
                        }
                        if (feof(cl->in)) {
                                fclose(cl->in);
                                if (cl->out) fclose(cl->out);
                                cl->in = NULL;
                                cl->out = NULL;
                        }
                }

                struct Client** prevp = &clients;
                while (*prevp) {
                        struct Client* curr = *prevp;
                        if(!curr->in) {
                                *prevp = curr->next;
                                free(curr);
                        } else {
                                prevp = &curr->next;
                        }
                }

 		bus_flush(bus, &wfds);
                bus_receive(bus, &rfds);
		bus_clocktick(bus);  // expire outstanding epos commands

		errno = 0;
                int changed = 0;

		for (i = LEFT; i <= RIGHT; ++i)
			if (dev[i] && !isnan(target_angles_deg[i])) {
                                double before = actual_angles_deg[i];
				dev_state[i] = rudder_control(dev[i], &motor_params[i],
							      target_angles_deg[i],
							      &actual_angles_deg[i]);
                                if (before != actual_angles_deg[i]) ++changed;
                        }

		if (dev[SAIL] && dev[BMMH] && !isnan(target_angles_deg[SAIL])) {
                        double before = actual_angles_deg[SAIL];
			dev_state[SAIL] = sail_control(dev[SAIL], dev[BMMH],
						       &motor_params[SAIL], &motor_params[BMMH],
						       target_angles_deg[SAIL],
                                                       &actual_angles_deg[SAIL]);
                        if (before != actual_angles_deg[SAIL]) ++changed;
                }

                if (changed) {
                        char line[1024];
                        char* l = line;
                        for (i = LEFT; i <= SAIL; ++i)
                                if (!isnan(actual_angles_deg[i]))
                                        l += snprintf(l, sizeof line - (l - line), "%s:%.2f ",
                                                      varnames[i], actual_angles_deg[i]);
                        if (l > line) {
                                l += snprintf(l, sizeof line - (l - line), "\n");
                                for (cl = clients; cl; cl = cl->next)
                                        if (cl->out)
                                                client_puts(cl, line);
                        }
                }

		// TODO if anything defunct or != TARGETREACHED for too long, tell the system manager

	}  // main loop

	bus_close(bus);

        crash("exit loop");

	return 0;
}
