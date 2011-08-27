// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
//  Commandline tool to read/write EPOS registers over RS232.
//

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "com.h"
#include "seq.h"

const char* version = "$Id: $";
const char* argv0;
int verbose=0;
int debug=0;


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
        fprintf(stderr, "usage: echo nodeid index subindex [:= value] | %s [-r] [-p] [-t timeout] /path/to/port\n", argv0);
        exit(1);
}

// Map serial numbers to nodeids.
static uint32_t nodeidmap[] = {
	-1, -1, -1, -1,
	-1, -1, -1, -1,
	-1, -1, /* -1, -1,
        -1, -1, -1, -1, */
};

#define nelem(x) (sizeof(x)/sizeof(x[0]))

int main(int argc, char* argv[]) {
	int ch;
	int raw = 0;
	int timeout_ms = 1000;
	int probe = 1;
	argv0 = argv[0];

	 while ((ch = getopt(argc, argv,"dhprt:v")) != -1){
		 switch (ch) {
		 case 'd': ++debug; break;
		 case 'p': probe=0; break;
		 case 'r': ++raw; break;
		 case 't': timeout_ms = atoi(optarg); break;
		 case 'v': ++verbose; break;
		 default:
			 usage();
		 }
	 }

	 argc -= optind;
	 argv += optind;

	 if (argc != 1) usage();

	 int fd = epos_open(argv[0]);
	 if (fd < 0) crash("epos_open(%s)", argv[0]);

	 if (probe) {
		 int nodeid;
		 printf("%s:", argv[0]);
		 for (nodeid = 1; nodeid < nelem(nodeidmap); ++nodeid) {
			 uint32_t err = epos_readobject(fd, 0x1018, 4, nodeid, nodeidmap + nodeid);
			 if (err != 0) continue;
			 fprintf(stderr, "port:%s nodeid:%d serial:0x%x\n", argv[0], nodeid, nodeidmap[nodeid]);
			 printf(" 0x%x", nodeidmap[nodeid]);
		 }
		 printf("\n");
		 fflush(stdout);
	 }

	 while (!feof(stdin)) {
		 char line[1024];
		 if (!fgets(line, sizeof(line), stdin)) continue;

		 if (line[0] == '#') continue;

		 int i = strlen(line) - 1;
		 while ((i > 0) && isspace(line[i])) line[i--] = 0;

		 uint32_t serial = 0;
		 int index     = 0;
		 int subindex  = 0;
		 int64_t value_l = 0;
		 int32_t value = 0;

		 int c1 = 0;
		 int c2 = 0;
		 int n = sscanf(line, "%i:%i[%i] %n:= %lli%n",
				&serial, &index, &subindex, &c1,
				&value_l, &c2);
		 // TBD maybe disallow n == 3 && line[c1] == ':' or '=',
		 // or stronger: require line[c1] == 0 or '#'

		 int nodeid = nelem(nodeidmap);
		 if (probe)
			 for(nodeid = 1; nodeid < nelem(nodeidmap); ++nodeid)
				 if (nodeidmap[nodeid] == serial)
					 break;

		 if (nodeid == nelem(nodeidmap)) nodeid = serial;

		 uint32_t err = 0;
		 switch (n) {
		 case 3: {
			 if (raw)
				 err = epos_readobject(fd, index, subindex, nodeid, (uint32_t*)&value);
			 else
				 err = epos_waitobject(fd, timeout_ms, index, subindex, nodeid, 0, (uint32_t*)&value);
			 const char* ec = epos_strerror(err);
			 printf("%.*s = 0x%x (0x%x) # %s%s\n", c1, line, value, err, ec, line[c1] ? line + c1 : "" );
			 break;
		 }

		 case 4: {
			 value = value_l;
			 if (raw) {
				 err = epos_writeobject(fd, index, subindex, nodeid, value);
			 } else {
				 struct EposCmd cmds[] = {
					 { index, subindex, value },
					 { 0, 0, 0 },
				 };
				 struct EposCmd* cmd = cmds;
				 err = epos_sequence(fd, nodeid, &cmd);
			 }
			 const char* ec = epos_strerror(err);
			 printf("%.*s (0x%x) # %s%s\n",	c2, line, err, ec, line[c2] ? line+c2 : "");
			 break;
		 }

		 default:
			 fprintf(stderr, "Unparsable line at field %d\n", n);
			 printf("## %s\n", line);
		 }
		 fflush(stdout);
	 }

	 return 0;
}
