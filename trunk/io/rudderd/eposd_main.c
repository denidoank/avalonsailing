// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Daemon to multiplex devices on can busses on epos controllers on
// serial ports to clients over unix sockets.
//
// or every /dev/ttyXXX listed on the commandline, forks off
// an eposcom, probes the serial numbers of all controller nodes on that bus,
// and enters the main loop.
//
// In the main loop
//     - accept connections from the control socket
//     - read messages from the control socket's client connections
//       and forward them 'atomically' to the right slave depending on
//       the device serial number
//     - read messages from the slave and copy them out to *all*
//       client connections.
//
//  writing to client and slaves is buffered.  if the buffer is full
//  (fputs returns EAGAIN), lines from slave to client are dropped!
//  lines from client to slave that cause the slave to block will block
//  the client.
#define _GNU_SOURCE

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
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

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

static void
usage(void)
{
        fprintf(stderr,
                "usage: %s [options]\n"
                "options:\n"
                "\t-E /path/to/eposcom (default $(dirname(argv[0])/eposcom)\n"
                "\t [boat parameters]\n"
                , argv0);
        exit(1);
}

static pid_t
popen2(char* cmd, FILE** ctl, FILE**sts, char* arg)
{
	int p_stdin[2], p_stdout[2];  // 0,1 = read,write
	if (pipe(p_stdin) || pipe(p_stdout)) crash("pipe");
	pid_t pid = fork();
	if (pid < 0) crash("fork");
	if (pid == 0) {		// child code
		close(p_stdin[1]);
		close(p_stdout[0]);
		dup2(p_stdin[0], 0);  // read end of p_stdin is my stdin
		dup2(p_stdout[1], 1);  // write end of p_stdout is my stdout
		execl(cmd, cmd, arg, (char*)NULL);
		crash("execl(%s %s)",  cmd, arg);
	}
	close(p_stdin[0]);
	close(p_stdout[1]);
	*ctl = fdopen(p_stdin[1],  "w");
	*sts = fdopen(p_stdout[0], "r");
	return pid;
}

static void set_fd(fd_set* s, int* maxfd, FILE* f) {
        int fd = fileno(f);
        FD_SET(fd, s);
        if (*maxfd < fd) *maxfd = fd;
}

struct PortSlave {
	const char* port;
	pid_t pid;
	FILE* ctl;
	FILE* sts;
        int pending;  // possibly unflushed lines in sts buffer
} *slaves;
int nslaves;

char* path_to_eposcom = NULL;
static void
slave_start(struct PortSlave* slave, char* port)
{
        slave->pid = popen2(path_to_eposcom, &slave->ctl, &slave->sts, port);
        slave->port = port;
        setbuffer(slave->ctl, NULL, 64<<10); //64k out buffer
        slave->pending = 0;
}

static void
slave_stop(struct PortSlave* slave)
{
        fclose(slave->ctl);
        fclose(slave->sts);
        slave->pid = 0;
        slave->ctl = NULL;
        slave->sts = NULL;
        slave->pending = 0;
}

static void slave_set_nonblocking(struct PortSlave* slave) {
        if (fcntl(fileno(slave->ctl), F_SETFL, O_NONBLOCK) < 0) crash("fcntl(ctl)");
        if (fcntl(fileno(slave->sts), F_SETFL, O_NONBLOCK) < 0) crash("fcntl(sts)");
}

// write line to slave and return EAGAIN if buffer full (and couldn't be flushed)
static int slave_puts(struct PortSlave* slave, char* line) {
        if (slave->ctl == NULL) return EOF;
        slave->pending = 1;
        if (fputs(line, slave->ctl) >= 0) return 0;
        if (feof(slave->ctl)) {
                slave_stop(slave);
                return EOF;
        }
        int e = ferror(slave->ctl);
        clearerr(slave->ctl);
        if (e == EWOULDBLOCK) e = EAGAIN;
        return e;
}

static void slave_flush(struct PortSlave* slave) {
        if (slave->ctl == NULL) return;
        if (fflush(slave->ctl) == 0)
                slave->pending = 0;
        if (feof(slave->ctl)) {
                slave_stop(slave);
                return;
        }
        clearerr(slave->ctl);
}

static volatile sig_atomic_t got_SIGCHLD = 0;
static void handle_SIGCHLD(int sig) { got_SIGCHLD = 1; }

static void reap_children() {
        pid_t pid;
        int stat, i;
        while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
                for (i = 0; i < nslaves; ++i)
                        if (slaves[i].pid == pid)
                                slave_stop(slaves+i);
}

struct SerialMap {
	uint32_t serial;
	struct PortSlave* slave;
} devices[16];
int ndevices = 0;


static struct PortSlave*
find_dest(const char* line)
{
        uint32_t serial;
        if (sscanf(line, "%i", &serial) != 1) return NULL;
        int i;
        for (i = 0; i < ndevices; ++i)
                if (serial == devices[i].serial)
                        return devices[i].slave;
        return NULL;
}

struct Client {
        struct Client* next;
        FILE* in;
        FILE* out;
        struct sockaddr_un addr;
        unsigned int addrlen;
        int pending;  // possibly unflushed data in out buffer
        char line[1024];  // last read line from in
        struct PortSlave* slave;  // destination of last line, if not yet written there
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
        cl->slave = 0;
        clients = cl;
        if (debug) fprintf(stderr, "new client %d\n", fd);
        return cl;
}

// write line to client.  return EAGAIN if buffer full (and couldn't be flushed)
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

	int ch;

	char* path_to_socket = NULL;

        argv0 = strrchr(argv[0], '/');
        if (argv0) ++argv0; else argv0 = argv[0];

        while ((ch = getopt(argc, argv, "dE:hS:v")) != -1){
                switch (ch) {
                case 'd': ++debug; break;
		case 'E': path_to_eposcom = optarg;
		case 'S': path_to_socket = optarg;
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

	if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

        // Set up socket
	unlink(path_to_socket);

	int sck = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sck < 0) crash("socket");
	struct sockaddr_un srvaddr = { AF_LOCAL, { 0 }};
	strncpy(srvaddr.sun_path, path_to_socket, sizeof(srvaddr.sun_path));
	if (bind(sck, (struct sockaddr*) &srvaddr, sizeof(srvaddr)) < 0)
                crash("bind");
	if (listen(sck, 8) < 0) crash("listen");

        if (debug) fprintf(stderr, "created socket:%s\n", path_to_socket);

        // Fork of slaves
	nslaves = argc;
	slaves = malloc(nslaves*sizeof(slaves[0]));

	int i;
	for (i = 0; i < nslaves; ++i) slave_start(slaves+i, argv[i]);

	// Parse the line with serial numbers
	for (i = 0; i < nslaves; ++i) {
		char line[1024];
		if (!fgets(line, sizeof(line), slaves[i].sts)) continue;
		char *p = strchr(line, ':');
		if (!p) continue;
		int n;
		while (sscanf(p+1, "%i %n", &devices[ndevices].serial, &n)==1) {
			devices[ndevices++].slave = slaves+i;
			p += n;
		}
	}

	for (i = 0; i<ndevices; ++i)
		if (debug)
			fprintf(stderr, "%s: Found device 0x%x on port %s\n", argv0, devices[i].serial, devices[i].slave->port);
		else
			syslog(LOG_INFO, "Found device 0x%x on port %s\n", devices[i].serial, devices[i].slave->port);

        // Reap any dead children (couldn't open serial or probe)
        reap_children();

#if 1
        // set to nonblocking after serial numbers have been read.
	for (i = 0; i < nslaves; ++i)
                if (slaves[i].pid)
                        slave_set_nonblocking(slaves+i);
#endif
        // Set up sigchld handling
        sigset_t sigmask;
        sigemptyset(&sigmask);
        sigaddset(&sigmask, SIGCHLD);
        sigaddset(&sigmask, SIGPIPE);
        if (sigprocmask(SIG_BLOCK, &sigmask, NULL) == -1) crash("sigprocmask");

        struct sigaction sa;
        sa.sa_flags = 0;
        sa.sa_handler = handle_SIGCHLD;
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGCHLD, &sa, NULL) == -1) crash("sigaction");

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

        // Main Loop
	for (;;) {
                struct timespec timeout = { 1, 0 }; // wake up at least 1/second
                fd_set rfds;
                fd_set wfds;
                FD_ZERO(&rfds);
                FD_ZERO(&wfds);

                FD_SET(sck, &rfds);
                int max_fd = sck;

                for (i = 0; i < nslaves; i++)
                        if(slaves[i].pid && slaves[i].pending)
                                set_fd(&wfds, &max_fd, slaves[i].ctl);

                for (i = 0; i < nslaves; i++)
                        if(slaves[i].pid)
                                set_fd(&rfds, &max_fd, slaves[i].sts);

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

                // fprintf(stderr, "woke up %d.\n", r);

                if (got_SIGCHLD) {
                        got_SIGCHLD = 0;
                        reap_children();
                        crash("slave died");
                }

                if (FD_ISSET(sck, &rfds)) new_client(sck);

                for (i = 0; i < nslaves; i++)
                        if (slaves[i].pid && FD_ISSET(fileno(slaves[i].ctl), &wfds))
                                slave_flush(slaves+i);

                for (cl = clients; cl; cl = cl->next)
                        if (cl->out && FD_ISSET(fileno(cl->out), &wfds))
                                client_flush(cl);

                for (i = 0; i < nslaves; i++)
                        if (slaves[i].pid && FD_ISSET(fileno(slaves[i].sts), &rfds)) {
                                char line[1024];
                                if (!fgets(line, sizeof(line), slaves[i].sts)) {
                                        // Read failure can only mean slave died.
                                        slave_stop(slaves+i);
                                        continue;
                                }
                                // Copy to all clients.  Silently drop on buffer overflow.
                                // TODO, keep count and report
                                for (cl = clients; cl; cl = cl->next)
                                        client_puts(cl, line);
                        }

                for (cl = clients; cl; cl = cl->next) {
                        if (!cl->in) continue;
                        // there is a pending line that could not be
                        // written previous time around, try it now,
                        // and only continue if it could be written.
                        if (cl->slave && (slave_puts(cl->slave, cl->line)!=0)) continue;

                        cl->slave = NULL;
                        if (cl->in && FD_ISSET(fileno(cl->in), &rfds)) {
                                while(fgets(cl->line, sizeof(cl->line), cl->in)) {
                                        cl->slave = find_dest(cl->line);
                                        if (!cl->slave) {
                                                // bad line, silently drop
                                                // TODO keep count and report
                                                continue;
                                        }
                                        // if we can't write it to the slave, block this client
                                        if (slave_puts(cl->slave, cl->line)!=0) break;
                                        cl->slave = NULL;
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

                // Write ping to sysmon, like in fm.cc
        } // main loop

	return 0;
}
