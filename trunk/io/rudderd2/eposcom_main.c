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
#include <sys/time.h>

#include "com.h"
#include "seq.h"
#include "../log.h"

//static const char* version = "$Id: $";
static const char* argv0;
static int verbose=0;
static int debug=0;

static void usage(void) {
        fprintf(stderr, "usage: echo nodeid index subindex [:= value] | %s [-r] [-T] [-t timeout] /path/to/port\n", argv0);
        exit(1);
}

static int64_t now_us() {
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) crash("no working clock");

        int64_t us1 = tv.tv_sec;  us1 *= 1000000;
        int64_t us2 = tv.tv_usec;
        return us1 + us2;
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
	int dotimestamps = 0;
	int timeout_ms = 1000;
	argv0 = argv[0];

	 while ((ch = getopt(argc, argv,"dhrTt:v")) != -1){
		 switch (ch) {
		 case 'd': ++debug; break;
		 case 'r': ++raw; break;
		 case 'T': ++dotimestamps; break;
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
		 // Read serial number register 0x1018[4]
		 uint32_t err = epos_readobject(fd, 0x1018, 4, nodeid, nodeidmap + nodeid);
		 if (err != 0) continue;
		 slog(LOG_INFO, "port:%s nodeid:%d serial:0x%x\n", argv[0], nodeid, nodeidmap[nodeid]);
		 // Ask linebusd to filter.  Note: we assume all clients will print serial in hex,
		 // which they will if they use eposclient.h EPOS_G/SET_OFMT
		 printf("$subscribe 0x%x\n", nodeidmap[nodeid]);
	 }

	 while (!feof(stdin)) {

		 char line[1024];
		 if (!fgets(line, sizeof(line), stdin))
			 crash("reading stdin");

		 uint32_t serial = 0;
		 int index       = 0;
		 int subindex    = 0;
		 char op[3] 	 = { 0, 0, 0 };
		 int64_t value_l = 0;
		 int32_t value   = 0;
		 uint32_t err 	 = 0;

		 int n = sscanf(line, "%i:%i[%i] %2s %lli", &serial, &index, &subindex, op, &value_l);

		 if(n < 3) continue;

		 // an ack or nack, must be from somewhere else
		 if(n >= 4 && (op[0] == '=' || op[0] == '#'))
			 continue;
		 // an incomplete  assignment.  drop now rather than interpret as get later.
		 if(n == 4 && (op[0] == ':' && op[1] == '='))
			 continue;

		 for(nodeid = 1; nodeid < nelem(nodeidmap); ++nodeid)
			 if (nodeidmap[nodeid] == serial)
				 break;
		 if (nodeid == nelem(nodeidmap))  // not for us
			 continue;
		 
		 if(n == 5 && op[0] == ':' && op[1] == '=') {
			 // a valid set request
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

		 } else { 
			 // we had at least 3 fields plus possibly
			 // some trailing garbage, but not ':=' '=' or '#',
			 // interpret it as a get request.
			 if (raw)
				 err = epos_readobject(fd, index, subindex, nodeid, (uint32_t*)&value);
			 else
				 err = epos_waitobject(fd, timeout_ms, index, subindex, nodeid, 0, (uint32_t*)&value);
		 }
		    
		 if (err) {  // no timestamps with nacks
			 printf("0x%x:0x%x[%d] # 0x%x: %s\n", serial, index, subindex, err, epos_strerror(err));
			 continue;
		 }

		 if(dotimestamps)
			 printf("0x%x:0x%x[%d] = 0x%x T:%lld\n", serial, index, subindex, value, now_us());
		 else
			 printf("0x%x:0x%x[%d] = 0x%x\n", serial, index, subindex, value);

	 }

	 crash("main loop exit");
	 return 0;
}
