// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Netcat like 'plug' for unix sockets.
//
//

#include <errno.h>
#include <getopt.h>
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
		"usage: %s [options] /path/to/socket\n"
		"options:\n"
		"\t-i input only\n"
		"\t-o output only\n"
		"\t-d debug (don't go daemon, don't syslog)\n"
		, argv0, argv0);
	exit(2);
}

int main(int argc, char* argv[]) {
	int ch;
	int noin = 0;
	int noout = 0;

	pid_t s_to_out_pid = -1;
	pid_t in_to_s_pid  = -1;

        argv0 = argv[0];

	while ((ch = getopt(argc, argv, "dhiov")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
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

        int s = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (s < 0) crash("socket");
        struct sockaddr_un addr;
        addr.sun_family = AF_LOCAL;
        strncpy(addr.sun_path, argv[0], sizeof(addr.sun_path));

        if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
                crash("connect(%s)", argv[0]);

	argc--; argv++;

	if (argc) {
		if (noin) close(0); else dup2(s, 0);
		if (noout) close(1); else dup2(s, 1); 
		execvp(argv[0], argv);
		crash("Failed to exec %s", argv[0]);
		return 0;
	}

	// Else, no command, but cat stdin and stdout linewise

	if (noout) {
		fclose(stdout);
	} else {
		s_to_out_pid = fork();
		if (s_to_out_pid == 0) {
			FILE* s_in = fdopen(s, "r");
			setlinebuf(stdout);
			char line[1024];
			while (!feof(s_in))
				if (fgets(line, sizeof(line), s_in))
					fputs(line, stdout);
			exit(0);
		}
		if (s_to_out_pid < 0) crash("fork(socket to out)");
	}

	if (noin) {
		fclose(stdin);
	} else {
		in_to_s_pid  = fork();
		if (in_to_s_pid == 0) {
			FILE* s_out = fdopen(s, "w");
			setlinebuf(s_out);
			char line[1024];
			while (!feof(stdin))
				if (fgets(line, sizeof(line), stdin))
					fputs(line, s_out);
			exit(0);
		}
		if (in_to_s_pid < 0) crash("fork(in to socket)");
	}

	if ((in_to_s_pid != -1) || (s_to_out_pid != -1)) {
			if (wait(NULL) < 0) crash("wait");
	}

	if (in_to_s_pid  != -1) kill(in_to_s_pid, SIGTERM);
        if (s_to_out_pid != -1) kill(s_to_out_pid, SIGTERM);

        return 0;
}
