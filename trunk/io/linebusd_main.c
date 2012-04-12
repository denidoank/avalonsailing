// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Daemon to read lines from stdin and publish them to any
// connected client.
//
//
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
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

#include "linebuffer.h"

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------
// static const char* version = "$Id: $";
static const char* argv0;
static int verbose = 0;
static int debug = 0;
static int timing = 0;

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

static void fault() { crash("fault"); }

static void
usage(void)
{
	fprintf(stderr,
		"usage: %s [options] /path/to/socket\n"
		"options:\n"
		"\t-d debug (don't go daemon, don't syslog)\n"
		, argv0);
	exit(2);
}

// -----------------------------------------------------------------------------
//   Linked list of open client sockets.
//     new_client    accepts a new connection from a listening socket
//     client_puts   writes a single line to the clients buffer
//     client_flush  tries to write out the output buffer, and detects eof
//     reap_clients  frees all closed clients from the list.
//
//   See the main loop below on sample usage.
// -----------------------------------------------------------------------------
static struct Client {
	struct Client* next;
	int fd;	// set to -1 when puts or flush detects EOF
	struct LineBuffer in;
	struct LineBuffer out;
	struct sockaddr_un addr;
	socklen_t addrlen;
	int dropped;
} *clients = NULL;

static struct Client*
new_client(int sck)
{
	struct Client* cl = (struct Client*)malloc(sizeof *cl);
	memset(cl, 0, sizeof(*cl));
	cl->addrlen = sizeof(cl->addr);
	cl->fd = accept(sck, (struct sockaddr*)&cl->addr, &cl->addrlen);
	if (cl->fd < 0) {
		// probably hung up before we got here, no reason to crash.
		if (debug) fprintf(stderr, "accept:%s\n", strerror(errno));
		free(cl);
		return NULL;
	}
	if (debug) fprintf(stderr, "New client: %d\n", cl->fd);
        if (fcntl(cl->fd,  F_SETFL, O_NONBLOCK) < 0) crash("fcntl(in)");
	cl->next = clients;
	clients = cl;
	return cl;
}

static void
set_fd(fd_set* s, int* maxfd, int fd)
{
	FD_SET(fd, s);
	if (*maxfd < fd) *maxfd = fd;
}

static void
client_setfds(struct Client* client, fd_set* rfds, fd_set* wfds, int* max_fd)
{
	if (client->fd < 0) return;
	if (lb_pending(&client->out)) set_fd(wfds, max_fd, client->fd);
	set_fd(rfds, max_fd, client->fd);
}

static void
client_puts(struct Client* client, const char* line)
{
	if (client->fd < 0) return;
	lb_putline(&client->out, line);
}

static void
client_flush(struct Client* client)
{
	if (client->fd < 0) return;
	if (!lb_pending(&client->out)) return;
	int r = lb_write(client->fd, &client->out);
	if ((r != 0) && (r != EAGAIN)) {
		close(client->fd);
		client->fd = -1;
	}
}

static const char*
client_gets(struct Client* client) {
	if (client->fd < 0) return NULL;
	int r = lb_read(client->fd, &client->in);
	if (r == 0) return client->in.line;
	if (r != EAGAIN) {
		close(client->fd);
		client->fd = -1;
	}
	return NULL;
}

static void
reap_clients()
{
	struct Client** prevp = &clients;
	while (*prevp) {
		struct Client* curr = *prevp;
		if(curr->fd < 0) {
			if (debug) fprintf(stderr, "Closed client.\n");
			*prevp = curr->next;
			free(curr);
		} else {
			prevp = &curr->next;
		}
	}
}


// -----------------------------------------------------------------------------
//   Main.
//   Parse options, open socket, optionally go daemon
//   and enter main loop.
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {

	int ch;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "dhtv")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
		case 't': ++timing; break;
		case 'v': ++verbose; break;
		case 'h':
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 1) usage();

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	signal(SIGBUS, fault);
        signal(SIGSEGV, fault);

	if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

	// Set up socket.
	unlink(argv[0]);
	int sck = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sck < 0) crash("socket");
	struct sockaddr_un srvaddr;
	memset(&srvaddr, 0, sizeof srvaddr);
	srvaddr.sun_family = AF_LOCAL;

	strncpy(srvaddr.sun_path, argv[0], sizeof srvaddr.sun_path );
	if (bind(sck, (struct sockaddr*) &srvaddr, sizeof srvaddr) < 0) crash("bind(%S)", argv[0]);
	if (listen(sck, 8) < 0) crash("listen");

	if (debug) fprintf(stderr, "created socket:%s\n", argv[0]);

	// Go daemon and write pidfile.
	if (!debug) {
		daemon(0,1);

		char* path_to_pidfile = NULL;
		asprintf(&path_to_pidfile, "%s.pid", argv[0]);
		FILE* pidfile = fopen(path_to_pidfile, "w");
		if(!pidfile) crash("writing pidfile");
		fprintf(pidfile, "%d\n", getpid());
		fclose(pidfile);
		free(path_to_pidfile);

		syslog(LOG_INFO, "Started.");
	}

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) crash("signal");

	// Main Loop
	for(;;) {

		fd_set rfds;
		fd_set wfds;
		int max_fd = -1;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		set_fd(&rfds, &max_fd, sck);

		struct Client* cl;
		for (cl = clients; cl; cl = cl->next)
			client_setfds(cl, &rfds, &wfds, &max_fd);

		sigset_t empty_mask;
		sigemptyset(&empty_mask);
		int r = pselect(max_fd + 1, &rfds, &wfds, NULL, NULL, &empty_mask);
		if (r == -1 && errno != EINTR) crash("pselect");

		if (debug > 1) fprintf(stderr, "woke up %d\n", r);

		if (FD_ISSET(sck, &rfds)) new_client(sck);

		for (cl = clients; cl; cl = cl->next)
			if (cl->fd >= 0 && FD_ISSET(cl->fd, &wfds))
				client_flush(cl);

		// Find first client that's ready for reading
		struct Client** cp;
		for (cp = &clients; *cp; cp = &(*cp)->next)
			if ((*cp)->fd >= 0 && FD_ISSET((*cp)->fd, &rfds))
				break;

		if(*cp) {
			const char* line = client_gets(*cp);
			if (!line) continue;
			while(*line == '\n') ++line;
			if (!line[0]) continue;
			if (debug) puts(line);

			for (cl = clients; cl; cl = cl->next) {
				if (cl == *cp) continue;
				if (cl->fd < 0) continue;
				if (lb_pending(&cl->out)) {
					cl->dropped++;
					if (debug) fprintf(stderr, "Dropping output to client %d.\n", cl->fd);
					continue;
				}
				client_puts(cl, line);
			}

			// move last read client to end of list
			cl = *cp;
			*cp = cl->next;
			cl->next = NULL;
			while(*cp)
				cp = &(*cp)->next;
			*cp = cl;
		}

		reap_clients();

	} // main loop

	crash("Terminating");

	return 0;
}
