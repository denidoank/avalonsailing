// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
//  Commandline tool to read/write EPOS registers over RS232.
//

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "com.h"
#include "seq.h"
#include "log.h"

//static const char* version = "$Id: $";
static const char* argv0;
static int verbose=0;
static int debug=0;

void usage(void) {
        fprintf(stderr, "usage: echo nodeid index subindex [:= value] | %s [-r] [-p] [-t timeout] /path/to/port\n", argv0);
        exit(1);
}

// Map serial numbers to nodeids.  We don't probe beyond number 9.
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
	argv0 = argv[0];

	 while ((ch = getopt(argc, argv,"dhrt:v")) != -1){
		 switch (ch) {
		 case 'd': ++debug; break;
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

	 setlinebuf(stdout);

	 openlog(argv0, debug?LOG_PERROR:0, LOG_LOCAL2);

	 if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	 if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");

	 int fd = epos_open(argv[0]);
	 if (fd < 0) crash("epos_open(%s)", argv[0]);

	 int nodeid;
	 for (nodeid = 1; nodeid < nelem(nodeidmap); ++nodeid) {
		 uint32_t err = epos_readobject(fd, 0x1018, 4, nodeid, nodeidmap + nodeid);
		 if (err != 0) continue;
		 slog(LOG_INFO, "port:%s nodeid:%d serial:0x%x\n", argv[0], nodeid, nodeidmap[nodeid]);
	 }

	 while (!feof(stdin)) {
		 char line[1024];
		 if (!fgets(line, sizeof(line), stdin))
			 crash("reading stdin");

		 if (line[0] == '#') continue;

		 uint32_t serial = 0;
		 int index       = 0;
		 int subindex    = 0;
		 int64_t value_l = 0;
		 int32_t value   = 0;

		 int c1 = 0;
		 int c2 = 0;
		 int n = sscanf(line, "%i:%i[%i] %n:= %lli %n",
				&serial, &index, &subindex, &c1,
				&value_l, &c2);

		 if (line[c1] && line[c1] != ':')  // someone elses ack or nack
			 continue;

		 for(nodeid = 1; nodeid < nelem(nodeidmap); ++nodeid)
			 if (nodeidmap[nodeid] == serial)
				 break;

		 if (nodeid == nelem(nodeidmap))  // not for us
			 continue;
			 
		 uint32_t err = 0;
		 switch (n) {
		 case 3: {
			 if (raw)
				 err = epos_readobject(fd, index, subindex, nodeid, (uint32_t*)&value);
			 else
				 err = epos_waitobject(fd, timeout_ms, index, subindex, nodeid, 0, (uint32_t*)&value);
			
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
			 break;
		 }

		 default: {
			 static int max = 100;
			 if (max-- > 0)
				 slog(LOG_WARNING, "Unparsable line at field %d:%s", n, line);
			 continue;
		 }

		 }  // switch

		 if (!err) {
			 printf("0x%x:0x%x[%d] = 0x%x\n", serial, index, subindex, value);
		 } else {
			 const char* ec = epos_strerror(err);
			 printf("0x%x:0x%x[%d] # 0x%x: %s\n", serial, index, subindex, err, ec);
		 }
	 }

	 crash("main loop exit");
	 return 0;
}
