// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Daemon to read lines from stdin and publish them to any
// connected client.
//
//
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

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------
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

static void
usage(void)
{
	fprintf(stderr,
		"usage: %s [options] /path/to/socket\n"
		"options:\n"
		"\t-d debug (don't go daemon, don't syslog)\n"
		, argv0, argv0);
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
	FILE* out;	// set to NULL when puts or flush detects EOF
	struct sockaddr_un addr;
	socklen_t addrlen;
	int pending;  // possibly unflushed data in out buffer
} *clients = NULL;

static struct Client*
new_client(int sck)
{
	struct Client* cl = (struct Client*)malloc(sizeof *cl);
	memset(cl, 0, sizeof(*cl));
	cl->addrlen = sizeof(cl->addr);
	int fd = accept(sck, (struct sockaddr*)&cl->addr, &cl->addrlen);
	if (fd < 0) {
		// probably hung up before we got here, no reason to crash.
		if (debug) fprintf(stderr, "accept:%s\n", strerror(errno));
		free(cl);
		return NULL;
	}
	if (debug) fprintf(stderr, "New client: %s\n", cl->addr.sun_path);
	cl->out = fdopen(fd, "w");
	if (fcntl(fileno(cl->out), F_SETFL, O_NONBLOCK) < 0) crash("fcntl(out)");
	setlinebuf(cl->out);
	cl->pending = 0;
	cl->next = clients;
	clients = cl;
	return cl;
}

static void
client_puts(struct Client* client, char* line)
{
	if (!client->out) return;
	client->pending = 1;
	if (fputs(line, client->out) >= 0) return;
	if (ferror(client->out)) {
		client->pending = 0;
		fclose(client->out);
		client->out = NULL;
	}
}

static void
client_flush(struct Client* client)
{
	if (!client->out) return;
	if (fflush(client->out) == 0)
		client->pending = 0;
	if (ferror(client->out)) {
		client->pending = 0;
		fclose(client->out);
		client->out = NULL;
	}
}

static void
reap_clients()
{
	struct Client** prevp = &clients;
	while (*prevp) {
		struct Client* curr = *prevp;
		if(!curr->out) {
			if (debug) fprintf(stderr, "Closed client: %s\n", curr->addr.sun_path);
			*prevp = curr->next;
			free(curr);
		} else {
			prevp = &curr->next;
		}
	}
}

// -----------------------------------------------------------------------------
// Little helper for main loop.
// -----------------------------------------------------------------------------
static void
set_fd(fd_set* s, int* maxfd, int fd)
{
	FD_SET(fd, s);
	if (*maxfd < fd) *maxfd = fd;
}

// -----------------------------------------------------------------------------
//   Main.
//   Parse options, open socket, optionally go daemon
//   and enter main loop.
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {

	int ch;
	char* path_to_socket = NULL;

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

	if (argc != 1) usage();

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

	if (fcntl(fileno(stdin), F_SETFL, O_NONBLOCK) < 0) crash("fcntl(stdin)");

	// Set up socket.
	unlink(argv[0]);
	int sck = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sck < 0) crash("socket");
	struct sockaddr_un srvaddr = { AF_LOCAL, 0 };
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
	while(!feof(stdin)) {

		fd_set rfds;
		fd_set wfds;
		int max_fd = -1;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		set_fd(&rfds, &max_fd, fileno(stdin));
		set_fd(&rfds, &max_fd, sck);

		struct Client* cl;
		for (cl = clients; cl; cl = cl->next)
			if (cl->out && cl->pending)
				set_fd(&wfds, &max_fd, fileno(cl->out));

		sigset_t empty_mask;
		sigemptyset(&empty_mask);
		int r = pselect(max_fd + 1, &rfds, &wfds, NULL, NULL, &empty_mask);
		if (r == -1 && errno != EINTR) crash("pselect");

		if (debug) fprintf(stderr, "woke up %d\n", r);

		// Handle clients.
		if (FD_ISSET(sck, &rfds)) new_client(sck);

		for (cl = clients; cl; cl = cl->next)
			if (cl->out && FD_ISSET(fileno(cl->out), &wfds))
				client_flush(cl);

		reap_clients();

		// Handle stdin.
		if (FD_ISSET(fileno(stdin), &rfds)) {
			char line[2048];
			while(fgets(line, sizeof line, stdin))
				for (cl = clients; cl; cl = cl->next)
					client_puts(cl, line);
			if (ferror(stdin)) {
				if (errno != EAGAIN)
					fprintf(stderr, "Reading stdin: %s\n", strerror(errno));
				clearerr(stdin);
				errno = 0;
			}
		}

	} // main loop

	crash("Terminating");

	return 0;
}
