// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Netcat like 'plug' for unix sockets. Useful for testing eposd.
//
//

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
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
#include <time.h>
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
                "usage: %s [-d] /path/to/socket ... \n"
                , argv0);
        exit(1);
}

static int
clsockopen(const char* path)
{
	int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (fd < 0) crash("socket");
	struct sockaddr_un addr = { AF_LOCAL, 0 };
	strncpy(addr.sun_path, path, sizeof(addr.sun_path));
	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		close(fd);
		return -1;
	}
	if (fcntl(fd,  F_SETFL, O_NONBLOCK) < 0) crash("fcntl(%s)", path);
	if (debug) fprintf(stderr, "opened %s\n", path);
	return fd;
}

static FILE*
fd_openr(int fd)
{
	if (fd < 0) return NULL;
	return fdopen(fd, "r");
}

static void
set_fd(fd_set* s, int* maxfd, FILE* f)
{
	if (!f) return;
        int fd = fileno(f);
        FD_SET(fd, s);
        if (*maxfd < fd) *maxfd = fd;
}


int main(int argc, char* argv[]) {

	int ch, i;

        argv0 = argv[0];

        while ((ch = getopt(argc, argv, "dE:hs:v")) != -1){
                switch (ch) {
                case 'd': ++debug; break;
                case 'v': ++verbose; break;
                case 'h':
                default:
                        usage();
                }
        }

	argv += optind; argc -= optind;
	
        if (!argc) crash("usage:%s /path/to/socket ...\n", argv0);

	setlinebuf(stdout);

	FILE **s = malloc(argc * sizeof *s);

	char line[2048];
    
	for (;;) {

		// Try to open any of the missing ones
		for (i = 0; i < argc; ++i)
			if (!s[i])
				s[i] = fd_openr(clsockopen(argv[i]));
		

		struct timespec timeout = { 2, 0 };
		fd_set rfds;
		int max_fd = -1;
		FD_ZERO(&rfds);

		for (i = 0; i < argc; ++i)
			set_fd(&rfds, &max_fd, s[i]);

		sigset_t empty_mask;
                sigemptyset(&empty_mask);
                int r = pselect(max_fd + 1, &rfds, NULL, NULL, &timeout, &empty_mask);
                if (r == -1 && errno != EINTR) crash("pselect");

		for (i = 0; i < argc; ++i)
			if (s[i] && FD_ISSET(fileno(s[i]), &rfds)) {
				while(fgets(line, sizeof line, s[i])) {
					if (verbose) printf("%s:", argv[i]);
					fputs(line, stdout);
				}
				if (ferror(s[i]) && (errno == EAGAIN)) {
					clearerr(s[i]);
				} else if (ferror(s[i]) || feof(s[i])) {
					fclose(s[i]);
					s[i] = NULL;
					if (debug) fprintf(stderr, "closed %s\n", argv[i]);
				}
			}
	}

        return 0;
}
