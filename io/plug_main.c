// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Netcat like 'plug' for unix sockets. Useful for testing eposd.
//
//

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

const char* argv0;

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

int main(int argc, char* argv[]) {
        argv0 = argv[0];
        if (argc != 2) crash("usage: %s /path/to/socket\n", argv0);

        int s = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (s < 0) crash("socket");
        struct sockaddr_un addr;
        addr.sun_family = AF_LOCAL;
        strncpy(addr.sun_path, argv[1], sizeof(addr.sun_path));

        if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
                crash("connect(%s)", argv[1]);

        pid_t s_to_out_pid = fork();
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

        pid_t in_to_s_pid  = fork();
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

        if (wait(NULL) < 0) crash("wait");

        kill(in_to_s_pid, SIGTERM);
        kill(s_to_out_pid, SIGTERM);

        return 0;
}
