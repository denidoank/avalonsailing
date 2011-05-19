// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
//  Commandline tool to read/write EPOS registers over RS232.
//

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "com.h"

const char* version = "$Id: $";
int verbose=0;
const char* argv0;

void crash(const char* fmt, ...) {
        va_list ap;
        char buf[1000];
        va_start(ap, fmt);
        vsnprintf(buf, 1000, fmt, ap);
        fprintf(stderr, "%s:%s%s%s\n", argv0, buf,
                (errno) ? ": " : "",
                (errno) ? strerror(errno):"" );
        exit(1);
        va_end(ap);
        return;
}

void usage(void) {
        fprintf(stderr, "usage: %s /path/to/port nodeid index subindex [=value]\n", argv0);
        exit(1);
}

int main(int argc, char* argv[]) {

        argv0 = argv[0];

        if (argc != 5 && argc != 6) usage();

        int fd = epos_open(argv[1]);
        if (fd < 0) crash("epos_open(%s)", argv[1]);

        int nodeid = strtol(argv[2], NULL, 0);
        if (errno) crash("invalid nodeid '%s'", argv[2]);

        int index = strtol(argv[3], NULL, 0);
        if (errno) crash("invalid index '%s'", argv[3]);

        int subindex = strtol(argv[4], NULL, 0);
        if (errno) crash("invalid subindex '%s'", argv[4]);

        if (argc == 6) {
                if (argv[5][0] != '=') usage();

                uint32_t value = strtoul(argv[5]+1, NULL, 0);
                if (errno) crash("invalid value '%s'", argv[5]);
                printf("%d:%4x[%x] := 0x%08x :",  nodeid, index, subindex, value);
                fflush(stdout);
                uint32_t err = epos_writeobject(fd, index, subindex, nodeid, value);
                printf("(0x%x) %s\n", err, epos_strerror(err));
                if (err != 0) crash("writeobject");
        } else {
                uint32_t value = 0;
                uint32_t err = epos_readobject(fd, index, subindex, nodeid, &value);
                printf("%d:%4x[%x] == ", nodeid, index, subindex);
                fflush(stdout);
                printf("0x%08x : (0x%x) %s\n", value, err, epos_strerror(err));
                if (err != 0) crash("readobject");
        }

        return 0;
}
