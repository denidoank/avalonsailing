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
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "log.h"

static const char* argv0;
static int debug = 0;

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
        int head;
        int discard;
        int tail;
} *msgs = NULL;


static int
samemsg(struct AISMsg *a, struct AISMsg *b)
{
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


// Reads from fd into the message, returns 0 if there is a \n terminated
// line, EAGAIN if there is an incomplete line, or errno from read(2)
// otherwise.  if the buffer overflows before EOL sets the discard flag which will
// cause the current line to be discarded until an EOL is found.
// the line will be 0-terminated.
static int
msg_read(int fd, struct AISMsg* lb)
{
        int n = read(fd, lb->line+lb->head, sizeof(lb->line) - lb->head - 1);
        switch (n) {
        case 0:
                return EOF;
        case -1:
                return errno;
        }

        lb->line[lb->head+n] = 0;
        char* eol = strchr(lb->line+lb->head, '\n');
        if (eol) {
                lb->head = 0;
                if (!lb->discard) return 0;  // lb->line starts with a complete line
                lb->discard = 0;
        }
        
        // no EOL || discard
        lb->head += n;
        if (lb->head == sizeof(lb->line) - 1) lb->discard = 1;
        if (lb->discard) lb->head = 0;
        return EAGAIN;
}

int main(int argc, char* argv[]) {

	int ch;
	int update_s = 5;
	int garbage_s = 3600;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "u:g:dh")) != -1){
		switch (ch) {
		case 'u': update_s = atoi(optarg); break;
		case 'g': garbage_s = atoi(optarg); break;
		case 'd': ++debug; break;
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
	memset(msg, 0, sizeof *msg);
	for(;;) {

		struct timespec timeout = { update_s, 0 };
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fileno(stdin), &rfds);
		sigset_t empty_mask;
                sigemptyset(&empty_mask);
                int r = pselect(fileno(stdin)+ 1, &rfds, NULL, NULL, &timeout, &empty_mask);
                if (r == -1 && errno != EINTR) crash("pselect");

		if (r == 1) {
			r = msg_read(fileno(stdin), msg);
			if (r == EOF) break;
			if (r == EAGAIN) continue;
			if (r != 0) crash("reading stdin");
			if (sscanf(msg->line, "ais: timestamp_ms:%lld mmsi:%d msgtype:%d ", &msg->timestamp_ms, &msg->mmsi, &msg->msgtype) < 3) {
				if (badcnt++ > 100) crash("Nothing but garbage on stdin: %s", msg->line);
				if (debug) fprintf(stderr, "Could not parse stdin:%s", msg->line);
				continue;
			}

			badcnt = 0;
			msgcnt++;
			msg->link = msgs;
			msgs = msg;

			msg = malloc(sizeof *msg);
			memset(msg, 0, sizeof *msg);

			// delete older versions
			struct AISMsg** pp = &msgs->link;
			while (*pp) {
				struct AISMsg* curr = *pp;
				if (samemsg(curr, msgs)) {
					*pp = curr->link;
					free(curr);
					msgcnt--;
				} else {
					pp = &(*pp)->link;
				}
			}
		}

		// clean to lowwatermark when above highwatermarkxs
		if (msgcnt > 3000) {  // highwatermark
			if (debug) fprintf(stderr, "Cleaning %d to low watermark\n", msgcnt);
			int lowwatermark = 2000; // lowwatermark
			struct AISMsg** pp;
			for (pp = &msgs; *pp && lowwatermark; pp=&(*pp)->link)
				lowwatermark--;
			while (*pp) {
				struct AISMsg* curr = *pp;
				*pp = curr->link;
				free(curr);
				msgcnt--;
			}
		}

		if (msgs->timestamp_ms < lastwritten_ms) // clock jumped back
			lastwritten_ms = 0;
	  
		if (lastwritten_ms + update_s*1000 < msgs->timestamp_ms) {
			if (debug) fprintf(stderr, "Flushing %d messages\n", msgcnt);

			FILE* tmp = fopen(tmppath, "w");
			if (!tmp) crash("opening %s", tmppath);

			struct AISMsg** pp;
			for (pp = &msgs; *pp; pp=&(*pp)->link) {
				if ((*pp)->timestamp_ms > msgs->timestamp_ms) // clock jumped back
					break;
				if ((*pp)->timestamp_ms + 1000*garbage_s < msgs->timestamp_ms)
					break;
				fputs((*pp)->line, tmp);
			}
			
			if (fclose(tmp)) crash("closing %s", tmppath);
			if (rename(tmppath, argv[0])) crash("renaming to %s", argv[0]);

			// garbage collect the rest
			while (*pp) {
				struct AISMsg* curr = *pp;
				*pp = curr->link;
				free(curr);
				msgcnt--;
			}
		}

		if (debug) fprintf(stderr, "Loop  %d messages\n", msgcnt);
	}

	crash("Exit loop");
	return 0;
}
