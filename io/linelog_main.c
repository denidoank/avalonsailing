// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Sample linebus input and keep n logs of l lines
//



#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib/linebuffer.h"
#include "lib/log.h"
#include "lib/timer.h"

static const char* argv0;
static int debug = 0;

static void
usage(void)
{
	fprintf(stderr,
		"usage: [plug -o -f XXX /var/run/lbus |] %s [options] outpattern.%%d.ext\n"
		"options:\n"
		"\t-d debug     (don't syslog, -dd opens port as plain file)\n"
		"\t-l 100       keep logs of this many lines\n"
       		"\t-n 3         keep this many logs.  outpattern must contain a %%d\n"
		"\t-t seconds   sample one line per this many seconds\n"
		"\t-s           use system time instead of timestamp_ms field (must be first field)\n"
		, argv0);
	exit(2);
}

int main(int argc, char* argv[]) {

	int ch;

	int log_size = 100;
	int log_count = 3;
	int sample_s = 10;
	int use_systime = 0;
	
	while ((ch = getopt(argc, argv, "dl:n:t:s")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
		case 'l': log_size = atoi(optarg); break;
		case 'n': log_count = atoi(optarg); break;
		case 't': sample_s = atoi(optarg); break;
		case 's': ++use_systime; break;
		case 'h':
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 1) usage();

	// pattern must contain exactly 1 %, and it must be a %d
	char* p = strchr(argv[0], '%');
	if (!p) usage();
	if (p[1] != 'd') usage();
	if (strchr(p+1, '%')) usage();

	signal(SIGBUS, fault);
        signal(SIGSEGV, fault);

	openlog(argv0, debug?LOG_PERROR:0, LOG_DAEMON);
	if(!debug) setlogmask(LOG_UPTO(LOG_NOTICE));

	struct LineBuffer lb;
	memset(&lb, 0, sizeof lb);

	int64_t next_ms = -1;
	int lc = 0;

	char fname[1024];
	snprintf(fname, sizeof fname, argv[0], 0);

	int of = open(fname, O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (of == -1) crash("open(%s, ...)", fname);

	for(;;) {
		int r = lb_readfd(&lb, fileno(stdin));
		if (r == EOF) break;
		if (r == EAGAIN) continue;

		char line[1024];
		while(lb_getline(line, sizeof line, &lb)) {

			int64_t now;
			char dum[25];
			if (use_systime) {
				now = now_ms();
			} else {
				int n = sscanf(line, "%s timestamp_ms:%lld", dum, &now);
				if (n != 2) crash("can not parse timestamp from input: \"%s\"", line);
			}

			if (next_ms == -1) next_ms = now;

			if(now < next_ms) continue;

			while(next_ms <= now) next_ms += 1000*sample_s;

			if (lc++ % log_size == 0) {
				close(of);

				int d;
				for(d = log_count - 1; d >= 0; --d) {
					char oname[1024];
					char nname[1024];
					snprintf(oname, sizeof oname, argv[0], d);
					snprintf(nname, sizeof nname, argv[0], d+1);
					rename(oname, nname);
				}

				of = open(fname, O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
				if (of == -1) crash("open(%s, ...)", fname);
			}

			char* p = line;
			int n = strlen(line);
			while (n) {
				int r = write(of, p, n);
				if (r == -1 && errno == EAGAIN) continue;
				if (r == -1) crash("write(%s...)", fname);
				n -= r;
				p += r;
			}
		}
	}
	crash("main loop exit");
	return 0;
}
