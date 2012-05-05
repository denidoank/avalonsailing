// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// LineBus daemon.
//
// The linebus daemon listens on a unix socket for connecting clients.
// A client can send lines (of up to 1Kb) which will then be copied out
// to all other listening clients.  Clients which are blocked for writing
// on their socket will be skipped, so delivery is not reliable.
// Lines prefixed with the command character (default '$') are handled by
// the daemon itself.
//
// The commands are
//    $id <identifier>     Name this client for diagnostic purposes
//    $stats               Echo to the client some statistics about all clients
//    $xoff		   don't send any further output to this client.
//    $subscribe <prefix>  Install a filter (see below)
//
// By default each client is eligible to receive all messages, but this can
// be changed by setting filters.  As soon as a client $subscribes to a <prefix>
// only lines matching that prefix will be forwarded to it. A client can subscribe
// to multiple prefixes.  Currently, there is no way to unsubscribe.
// 
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
		"\t-d debug   (don't go daemon, don't syslog)\n"
		"\t-c cmdchar command prefix character (default '$')\n"
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
	char* name;
	int xoff;
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
		syslog(LOG_INFO, "accept:%s", strerror(errno));
		free(cl);
		return NULL;
	}
	syslog(LOG_INFO, "New client: %d", cl->fd);
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

static int
client_write(struct Client* client)
{
	if (client->fd < 0) return 0;
	int r = lb_writefd(client->fd, &client->out);
	if ((r != 0) && (r != EAGAIN)) {
		close(client->fd);
		client->fd = -1;
	}
	return r;
}

static int
client_read(struct Client* client) {
	if (client->fd < 0) return 0;
	int r = lb_readfd(&client->in, client->fd);
	if (r != 0 && r != EAGAIN) {
		close(client->fd);
		client->fd = -1;
	}
	return r;
}

static void
reap_clients()
{
	struct Client** prevp = &clients;
	while (*prevp) {
		struct Client* curr = *prevp;
		if(curr->fd < 0) {
			syslog(LOG_NOTICE, "Closed client %s.\n", curr->name ? curr->name : "<anon>");
			*prevp = curr->next;
			free(curr);
		} else {
			prevp = &curr->next;
		}
	}
}

static int client_puts(struct Client* client, const char* line){ return lb_putline(&client->out, (char*)line); }
static int client_gets(struct Client* client, char* line, int size) { return lb_getline(line, size, &client->in); }

static void
handle_cmd(struct Client* client, char* line) {
	int i;
	for(i = strlen(line) - 1; i >= 0 && line[i] == '\n'; --i)
		line[i] = 0;

	if (strncmp("id ", line, 3) == 0) {
		if(client->name) {
			syslog(LOG_NOTICE, "Client %d renamed '%s' from '%s'", client->fd, line + 3, client->name);
			free(client->name);
		} else {
			syslog(LOG_NOTICE, "Client %d named '%s'", client->fd, line + 3);
		}
		client->name = strndup(line+3, 20);
		return;
	}

	if (strcmp("xoff", line) == 0) {
		client->xoff = 1;
		syslog(LOG_NOTICE, "Client %s (%d) set xoff\n", client->name ? client->name : "<anon>", client->fd);
		return;
	}

	if (strncmp("subscribe ", line, 10) == 0) {
		syslog(LOG_NOTICE, "Client %s (%d) subscribed:'%s'\n", client->name ? client->name : "<anon>", client->fd, line + 10);
		return;
	}
	
}


// -----------------------------------------------------------------------------
//   Main.
//   Parse options, open socket, optionally go daemon
//   and enter main loop.
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {

	int ch;
	int cmdchar = '$';

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "c:dhtv")) != -1){
		switch (ch) {
		case 'c': cmdchar = optarg[0]; break;
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

	openlog(argv0, debug?LOG_PERROR:0, LOG_DAEMON);
	if(!debug) setlogmask(LOG_UPTO(LOG_NOTICE));

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
	}

	syslog(LOG_NOTICE, "Started on socket %s", argv[0]);

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

		if(debug > 1) syslog(LOG_DEBUG, "woke up %d\n", r);

		int notdonewriting = 0;
		for (cl = clients; cl; cl = cl->next)
			if (cl->fd >= 0 && FD_ISSET(cl->fd, &wfds))
				if(client_write(cl) == EAGAIN)
					++notdonewriting;

		if(notdonewriting) { 		// blocked ones won't have been counted
			syslog(LOG_DEBUG, "Not done writing: %d", notdonewriting);
			continue;
		}

		// Find first client that's ready for reading
		struct Client** cp;
		for (cp = &clients; *cp; cp = &(*cp)->next)
			if ((*cp)->fd >= 0 && FD_ISSET((*cp)->fd, &rfds))
				if(client_read(*cp) == 0)   // at least 1 line is ready
					break;

		if(*cp) {
			char buf[1024];
			while(client_gets(*cp, buf, sizeof buf) > 0) {
				if(buf[0] == 0) continue;
				
				if(debug > 1 && buf[0]) fprintf(stdout, "received:>>%s<<", buf);

				if(buf[0] == cmdchar) {
					handle_cmd(*cp, buf+1);
					continue;
				} 

				for (cl = clients; cl; cl = cl->next) {
					if (cl == *cp) continue;
					if (cl->fd < 0) continue;
					if (cl->xoff) continue;
					if (client_puts(cl, buf) < 0) {
						cl->dropped++;
						syslog(LOG_DEBUG, "Dropping output to client %d.\n", cl->fd);
					}
				}
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
		// Accept new clients now.
		if (FD_ISSET(sck, &rfds)) new_client(sck);

	} // main loop

	crash("Terminating");

	return 0;
}
