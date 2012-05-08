// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Netcat like 'plug' for unix sockets.
//
//

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

static const char* argv0;
static int debug;
static int verbose;

static void
crash(const char* fmt, ...)
{
        va_list ap;
        char buf[1000];
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        fprintf(stderr, "%s:%s%s%s\n", argv0, buf,
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
		"usage: %s [options] /path/to/socket [command [opts]]\n"
		"options:\n"
		"\t-i input to socket only\n"
		"\t-o output from socket only\n"
		"\t-f subscription (may be repeated)\n"
		"\t-n name  diagnostic name for linebusd\n"
		"\t-d debug (don't go daemon, don't syslog)\n"
		"\t-c cmdchar linebusd uses alternate command character (default '$')\n"
		, argv0);
	exit(2);
}

struct List {
	struct List *next;
	char *str;
} *subscriptions = NULL;

struct List* NewItem(struct List* l, char* s) { 
	struct List *n = malloc(sizeof *n);
	n->next = l;
	n->str = s;
	return n;
}

static void fdcopy(int dst, int src) {
	char buf[2048];
	int r, w;
	while((r=read(src, buf, sizeof buf)) > 0) {
		char *p = buf;
		while(r>0) {
			w = write(dst, p, r);
			if (w < 0) return;
			p += w;
			r -= w;
		}
	}
}

int main(int argc, char* argv[]) {
	int ch;
	int noin = 0;
	int noout = 0;
	int cmdchar = '$';
	char *name = NULL;

	char cmdbuf[100];

	pid_t s_to_out_pid = -1;
	pid_t in_to_s_pid  = -1;

        argv0 = argv[0];

	while ((ch = getopt(argc, argv, "c:df:hin:ov")) != -1){
		switch (ch) {
		case 'c': cmdchar = optarg[0]; break;
		case 'n': name = optarg; break;
		case 'd': ++debug; break;
		case 'f': subscriptions = NewItem(subscriptions, optarg); break;
		case 'v': ++verbose; break;
		case 'o': ++noin; break;
		case 'i': ++noout; break;
		case 'h':
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc < 1) usage();

	if (noin && noout) usage();

        int s = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (s < 0) crash("socket");
        struct sockaddr_un addr;
        addr.sun_family = AF_LOCAL;
        strncpy(addr.sun_path, argv[0], sizeof(addr.sun_path));

        if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
                crash("connect(%s)", argv[0]);

	argc--; argv++;
	if(argc > 0 && argv[0][0] == '-' && argv[0][1] == '-') {
			argc--; argv++;
	}

	// Send commands to linebusd
	if(noout) {
		int n = snprintf(cmdbuf, sizeof cmdbuf, "%cxoff\n", cmdchar);
		write(s, cmdbuf, n);
	}

	if(name) {
		int n = snprintf(cmdbuf, sizeof cmdbuf, "%cname %s\n", cmdchar, name);
		write(s, cmdbuf, n);
	}

	struct List *sbs;
	for(sbs = subscriptions; sbs; sbs = sbs->next) {
		int n = snprintf(cmdbuf, sizeof cmdbuf, "%csubscribe %s\n", cmdchar, sbs->str);
		write(s, cmdbuf, n);		
	}

	if (argc) {
		if (noout) close(0); else dup2(s, 0);  // what child reads from socket 
		if (noin) close(1); else dup2(s, 1);   // what child writes to socket
		execvp(argv[0], argv);
		crash("Failed to exec %s", argv[0]);
		return 0;
	}

	// Else, no command, but copy stdin and stdout.

	if (!noout) {
		s_to_out_pid = fork();
		if (s_to_out_pid == 0) {
			fclose(stdin);
			fdcopy(fileno(stdout), s);
			exit(0);
		}
		if (s_to_out_pid < 0) crash("fork(socket to out)");
	}

	if (!noin) {
		in_to_s_pid  = fork();
		if (in_to_s_pid == 0) {
			fclose(stdout);
			fdcopy(s, fileno(stdin));
			exit(0);
		}
		if (in_to_s_pid < 0) crash("fork(in to socket)");
	}

	fclose(stdout);
	fclose(stdin);

	if ((in_to_s_pid != -1) || (s_to_out_pid != -1)) {
			if (wait(NULL) < 0) crash("wait");
	}

	if (in_to_s_pid  != -1) kill(in_to_s_pid, SIGTERM);
        if (s_to_out_pid != -1) kill(s_to_out_pid, SIGTERM);

        return 0;
}
