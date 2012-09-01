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
//    $name <identifier>   Name this client for diagnostic purposes
//    $stats               Echo to the client some statistics about all clients
//    $xoff		   don't send any further output to this client.
//    $subscribe <prefix>  Install a filter (see below)
//    $precious		   when this client exits or hangs, take down the bus
//    $kill <identifier>   close any client with this name (name uniqueness is not enforced)
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

#include "lib/linebuffer.h"
#include "lib/log.h"
#include "lib/timer.h"

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------
// static const char* version = "$Id: $";
static const char* argv0;
static int verbose = 0;
static int debug = 0;
static int timing = 0;

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
//   Prefix filters for subscriptions
// -----------------------------------------------------------------------------
static struct Filter {
	struct Filter* next;
	const char *pfx;
	int pfxlen;
	int refcount;
	int match;  
} *filters = NULL;

static struct Filter*
new_filter(const char *pfx)
{
	struct Filter* f;
	for (f = filters; f; f=f->next)
		if(strcmp(pfx,f->pfx)==0) {
			f->refcount++;
			return f;
		}

	f = malloc(sizeof *f);
	memset(f, 0, sizeof *f);
	f->pfx = strdup(pfx);
	f->pfxlen = strlen(f->pfx);
	f->next = filters;
	filters = f;
	return f;
}

static void
filter_match(const char* line)
{ 	struct Filter* f;
	for (f = filters; f; f=f->next)
		f->match = strncmp(line, f->pfx, f->pfxlen) ? 0 : 1;
}

static void
reap_filters()
{
	struct Filter** prevp = &filters;
	while (*prevp) {
		struct Filter* curr = *prevp;
		if(curr->refcount < 0) {
			syslog(LOG_DEBUG, "deleting filter '%s'", curr->pfx);
			*prevp = curr->next;
			free(curr);
		} else {
			prevp = &curr->next;
		}
	}
}

struct FilterList {
	struct FilterList* next;
	struct Filter* f;
};

static struct FilterList*
add_filter(struct FilterList* l, struct Filter* f)
{
	struct FilterList* ll = malloc(sizeof *ll);
	ll->next = l;
	ll->f = f;
	return ll;
}

static void
free_filters(struct FilterList* l)
{
	if(!l) return;
	free_filters(l->next);
	l->f->refcount--;
	free(l);
}

static int
filter_hit(struct FilterList* l)
{
	for( ; l; l = l->next)
		if(l->f->match)
			return 1;
	return 0;
}

// -----------------------------------------------------------------------------
//   Linked list of open client sockets.
//     new_client    accepts a new connection from a listening socket
//     client_read   fills the input buffer with whatever could be found on the socket
//     client_write  tries to empty the output buffer line by line
//     client_puts   writes a single line to the clients output buffer
//     client_gets   reads a signle line from the clients input buffer
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
	int precious;
	int dropped;
	struct FilterList* filters;
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
	int r = lb_writefd_all(client->fd, &client->out);
	if ((r != 0) && (r != EAGAIN)) {
		close(client->fd);
		client->fd = -1;
	}
	return r;
}

static int
client_read(struct Client* client)
{
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
		if (curr->fd < 0) {
			if (curr->precious) {
				syslog(LOG_WARNING, "Closed precious client %s, shutting down.\n", curr->name ? curr->name : "<anon>");
				crash("lost precious client.");
			}
			syslog(LOG_NOTICE, "Closed client %s.\n", curr->name ? curr->name : "<anon>");
			*prevp = curr->next;
			free_filters(curr->filters);
			free(curr);
		} else {
			prevp = &curr->next;
		}
	}
	return;
}

static int client_puts(struct Client* client, const char* line){ return lb_putline(&client->out, (char*)line); }
static int client_gets(struct Client* client, char* line, int size) { return lb_getline(line, size, &client->in); }

static const char* client_name(struct Client* cl) { return cl->name ? cl->name : "<anon>"; }

// -----------------------------------------------------------------------------
//   Handle $cmd lines
// -----------------------------------------------------------------------------
static void
handle_cmd(struct Client* client, char* line) {
	int i;
	for(i = strlen(line) - 1; i >= 0 && line[i] == '\n'; --i)
		line[i] = 0;

	if (strncmp("name ", line, 5) == 0) {
		if(client->name) {
			syslog(LOG_NOTICE, "Client %d renamed '%s' from '%s'", client->fd, line + 5, client->name);
			free(client->name);
		} else {
			syslog(LOG_NOTICE, "Client %d named '%s'", client->fd, line + 5);
		}
		client->name = strndup(line+5, 20);
		return;
	}

	if (strncmp("kill ", line, 5) == 0) {
		syslog(LOG_NOTICE, "Client %d killing '%s'", client->fd, line + 5);
		struct Client* cl;
		for(cl = clients; cl; cl=cl->next) {
			if (!strcmp(cl->name, line+5)) {
				close(cl->fd);
				cl->fd = -1;
			}
		}
		return;
	}

	if (strcmp("xoff", line) == 0) {
		client->xoff = 1;
		syslog(LOG_NOTICE, "Client %s (%d) set xoff\n", client_name(client), client->fd);
		return;
	}

	if (strcmp("xon", line) == 0) {
		client->xoff = 0;
		syslog(LOG_NOTICE, "Client %s (%d) set xon\n", client_name(client), client->fd);
		return;
	}

	if (strcmp("precious", line) == 0) {
		client->precious = 1;
		syslog(LOG_NOTICE, "Client %s (%d) set precious\n", client_name(client), client->fd);
		return;
	}

	if (strcmp("stats", line) == 0) {
		char buf[1024];
		struct Client* cl;
		// TODO: per client read/write xstimes
		for(cl = clients; cl; cl=cl->next) {
			snprintf(buf, sizeof buf, "%d %s dropped: %d\n", cl->fd, client_name(cl), cl->dropped); 
			client_puts(client, buf);
		}
		return;
	}

	if (strncmp("subscribe ", line, 10) == 0) {
		syslog(LOG_NOTICE, "Client %s (%d) subscribed:'%s'\n", client_name(client), client->fd, line + 10);
		client->filters = add_filter(client->filters, new_filter(line+10));
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

	struct Timer timer;
	memset(&timer, 0, sizeof timer);
	
	timer_tick_now(&timer, 1);

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
		if(timer_tick_now(&timer, 0) > (debug?200:4000)) {  // 200us should do, but lets not spam the log
			struct TimerStats stats;
			timer_stats(&timer, &stats);
			slog((debug?LOG_DEBUG:LOG_WARNING), "slow cycle: " OFMT_TIMER_STATS(stats));
		}

		int r = pselect(max_fd + 1, &rfds, &wfds, NULL, NULL, &empty_mask);
		if (r == -1 && errno != EINTR) crash("pselect");
		timer_tick_now(&timer, 1);
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
			char line[1024];
			while(client_gets(*cp, line, sizeof line) > 0) {
				char* buf = line;
				while(*buf == '\n') buf++;   // skip empty lines
				if(buf[0] == 0) continue;
				
				if(debug > 1 && buf[0]) fprintf(stdout, "received:>>%s<<", buf);

				if(buf[0] == cmdchar) {
					handle_cmd(*cp, buf+1);
					continue;
				}

				filter_match(buf);

				for (cl = clients; cl; cl = cl->next) {
					if (cl == *cp) continue;
					if (cl->fd < 0) continue;
					if (cl->xoff) continue;
					if (cl->filters && !filter_hit(cl->filters)) continue;
					if (client_puts(cl, buf) < 0) {
						cl->dropped++;
						if (cl->dropped % 10 == 0)
							syslog(LOG_DEBUG, "Client %s (%d) dropped %d messages\n", client_name(cl), cl->fd, cl->dropped);
						if (cl->precious && cl->dropped > 100) { // drop 100 messages and you're hung
							syslog(LOG_WARNING, "Assuming client %s (%d) is hung\n", client_name(cl), cl->fd);
							close(cl->fd);
							cl->fd = -1;
						}
					} else
						cl->dropped >>= 1;  // halve the dropped count, so the client has a chance to slowly catch up.
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

		// Reap old and accept new clients.
		reap_clients();  // will crash on losing precious client.
		reap_filters();
		if (FD_ISSET(sck, &rfds)) new_client(sck);

	} // main loop

	crash("Terminating");

	return 0;
}
