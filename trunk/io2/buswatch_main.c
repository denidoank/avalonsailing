// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

// Read messages from stdin and every -t seconds, run command with 
// as arguments all prefixes we have not seen at least once during the interval.

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "lib/log.h"
#include "lib/timer.h"
#include "lib/linebuffer.h"

static const char* argv0;
static int debug = 0;

static void
usage(void)
{
	fprintf(stderr,
		"usage: [plug -o /path/to/bus |] %s [options] {/path/to/cmd| \"DIE\"} pfx [pfx...]\n"
		"options:\n"
		"\t-d debug\n"
		"\t-t seconds  interval to check over\n"
		, argv0);
	exit(2);
}

static int
startswith(const char* pfx, const char* s) {
        while(*pfx)
                if (*pfx++ != *s++)
                        return 0;
        return 1;
}

int main(int argc, char* argv[]) {

	int ch;
	int timeout_s = 30;
	
	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "dht:")) != -1){
		switch (ch) {
		case 't': timeout_s = atoi(optarg); break;
		case 'd': ++debug; break;
		case 'h': 
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc < 2) usage();

	signal(SIGBUS, fault);
        signal(SIGSEGV, fault);

	openlog(argv0, (debug?LOG_PERROR:0), LOG_DAEMON);
	if(!debug) setlogmask(LOG_UPTO(LOG_NOTICE));

	int* counts = malloc(argc * sizeof counts[0]);
	memset(counts, 0, argc * sizeof counts[0]);

	char** outargs = malloc(argc * sizeof outargs[0]);
	memset(outargs, 0, (argc+1) * sizeof outargs[0]);

	int64_t now = now_ms();
	int64_t next_run = now + timeout_s * 1000;

	struct LineBuffer lbuf;
	memset(&lbuf, 0, sizeof lbuf);

	for (;;) {

		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fileno(stdin), &rfds);

		struct timespec timeout = { 0, 0 };
		if (now < next_run) {
			timeout.tv_sec =   (next_run - now) / 1000;
			timeout.tv_nsec = ((next_run - now) % 1000) * 1000000;
		}
		sigset_t empty_mask;
		sigemptyset(&empty_mask);

		int r = pselect(fileno(stdin) + 1, &rfds,  NULL, NULL, &timeout, &empty_mask);
		if (r == -1 && errno != EINTR) crash("pselect");

		now = now_ms();

		if (debug>2) syslog(LOG_DEBUG, "Woke up %d\n", r);

		if (r == 1) {
			r = lb_readfd(&lbuf, fileno(stdin));
			if (r == EOF) break;
			if (r == EAGAIN) continue;
			if (r != 0) crash("reading stdin");
		}

		char line[1024];
		while(lb_getline(line, sizeof line, &lbuf) > 0) {
			int i;
			for(i = 1; i < argc; ++i)
				if(startswith(argv[i], line))
					break;
			if(i < argc) {
				counts[i]++;
				syslog(LOG_DEBUG, "got message '%s'", argv[i]);
			}
		}

		if (now < next_run) continue;

		int i = 0;
		int j = 0;
		outargs[i++] = argv[j++];
		for( ; j < argc; ++j)
			if (!counts[j]) {
				syslog(LOG_ERR, "message '%s' not seen for at least %d seconds", argv[j], timeout_s);
				outargs[i++] = argv[j];
			}
		outargs[i] = NULL;

		memset(counts, 0, argc*sizeof counts[0]);

		if(i > 1) {
			if(strcmp(outargs[0], "DIE")==0) crash("buswatch commiting suicide");
		  
			syslog(LOG_ERR, "buswatch invoking %s %s...", outargs[0], outargs[1]);
			pid_t pid = fork();
			if (pid < 0) crash("fork");
			if(pid == 0) {
				execvp(outargs[0], outargs);
				crash("Failed to exec %s", outargs[0]);
			}
			wait(NULL);
		}

		now = now_ms();
		while(now >= next_run) next_run += timeout_s * 1000;


	}
	return 0;
}
