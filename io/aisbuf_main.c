// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Read aiscat output on stdin and keep the last line per mssid/message type, 
// discard anything older than an hour, and update /var/run/ais.txt
// at most once per 10 seconds.
// 

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
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

static void fault() { crash("fault"); }

static void
usage(void)
{
	fprintf(stderr,
		"usage: [aiscat /dev/ttyXXX |] %s [options] outfile\n"
		"options:\n"
		"\t-d debug            (don't syslog, -dd opens port as plain file)\n"
		"\t-u update_s        write at most every this seconds\n"
       		"\t-g garbage_s       garbage collect messages older than this many seconds\n"
		, argv0);
	exit(2);
}

static struct AISMsg {
	struct AISMsg* link;
	int64_t timestamp_ms;
	int mmsi;
	int msgtype;
	char line[1024];
} *msgs = NULL;

static int samemsg(struct AISMsg *a, struct AISMsg *b) {
	if (a->mmsi != b->mmsi) return 0;
	if (a->msgtype == b->msgtype) return 1;
	switch(a->msgtype) {
	case 1: case 2: case 3:
		switch(b->msgtype) {
		case 1: case 2: case 3:
			return 1;
		}
	}
	return 0;
}


int main(int argc, char* argv[]) {

	int ch;
	int update_s = 5;
	int garbage_s = 3600;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "u:g:dhv")) != -1){
		switch (ch) {
		case 'u': update_s = atoi(optarg); break;
		case 'g': garbage_s = atoi(optarg); break;
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

	signal(SIGBUS, fault);
        signal(SIGSEGV, fault);

	if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);
	char tmppath[1024];
	snprintf(tmppath, sizeof tmppath, "%s.tmp", argv[0]);

	int64_t lastwritten_ms = 0;

	int badcnt = 0;
	int msgcnt = 0;
	struct AISMsg* msg = malloc(sizeof *msg);
	while (!feof(stdin)) {

		msg->link = msgs;

		if (!fgets(msg->line, sizeof msg->line, stdin))
			crash("reading stdin");

		if (sscanf(msg->line, "ais: timestamp_ms:%lld mmsi:%d msgtype:%d ", &msg->timestamp_ms, &msg->mmsi, &msg->msgtype) < 3) {
			if (badcnt++ > 100) crash("Nothing but garbage on stdin: %s", msg->line);
			if (debug) fprintf(stderr, "Could not parse stdin:%s", msg->line);
			continue;
		}

		badcnt = 0;
		msgcnt++;
		msgs = msg;

		struct AISMsg** pp = &msg->link;
		while (*pp) {
			struct AISMsg* curr = *pp;
			if (samemsg(curr, msg)) {
				*pp = curr->link;
				free(curr);
				msgcnt--;
			} else {
				pp = &(*pp)->link;
			}
		}

		if (msgcnt > 3000) {  // highwatermark
			if (debug) fprintf(stderr, "Cleaning %dto low watermark\n", msgcnt);
			int lowwatermark = 2000; // lowwatermark
			for (pp = &msgs; *pp && lowwatermark; pp=&(*pp)->link)
				lowwatermark--;
			while (*pp) {
				struct AISMsg* curr = *pp;
				*pp = curr->link;
				free(curr);
				msgcnt--;
			}
		}

		if (msg->timestamp_ms < lastwritten_ms) // clock jumped back
			lastwritten_ms = 0;
	  
		if (lastwritten_ms + garbage_s*1000 < msg->timestamp_ms) {
			if (debug) fprintf(stderr, "Flushing %d messages\n", msgcnt);

			FILE* tmp = fopen(tmppath, "w");
			if (!tmp) crash("opening %s", tmppath);

			for (pp = &msgs; *pp; pp=&(*pp)->link) {
				if ((*pp)->timestamp_ms > msg->timestamp_ms) // clock jumped back
					break;
				if ((*pp)->timestamp_ms + 1000*garbage_s < msg->timestamp_ms)
					break;
				fputs((*pp)->line, tmp);
			}
			
			if (fclose(tmp)) crash("closing %s", tmppath);
			if (rename(tmppath, argv[0])) crash("renaming to %s", argv[0]);

			while (*pp) {
				struct AISMsg* curr = *pp;
				*pp = curr->link;
				free(curr);
				msgcnt--;
			}
		}

		if (debug) fprintf(stderr, "Loop  %d messages\n", msgcnt);
		msg = malloc(sizeof *msg);
	}

	crash("Exit loop");
	return 0;
}
