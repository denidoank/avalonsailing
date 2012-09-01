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
#include <libgen.h>
#include <sys/time.h>
#include <unistd.h>

#include "com.h"
#include "seq.h"
#include "lib/log.h"
#include "lib/timer.h"
#include "ebus.h"

//static const char* version = "$Id: $";
static const char* argv0;
static int debug=0;

static void usage(void) {
        fprintf(stderr, "usage: echo nodeid index subindex [:= value] | %s [-r] [-T] [-t timeout] /path/to/port\n", argv0);
        exit(1);
}

// adapted from OFMT_TIMER_STATS
#define TIMER_OFMT(serial, s)		\
	"serial: 0x%x count:%lld   f(Hz): %.3lf dc(%%): %.1lf  period(ms): %.3lf / %.3lf (±%.3lf) / %.3lf  run(ms): %.3lf / %.3lf (±%.3lf) / %.3lf", \
		serial, (s).count, (s).f, (s).davg*100,			\
		(s).pmin/1000, (s).pavg/1000, (s).pdev/1000, (s).pmax/1000, \
		(s).rmin/1000, (s).ravg/1000, (s).rdev/1000, (s).rmax/1000

#define nelem(x) (sizeof(x)/sizeof(x[0]))

// Map serial numbers to nodeids.  We don't probe beyond number 9.
static uint32_t nodeidmap[] = {
	-1, -1, -1, -1,
	-1, -1, -1, -1,
	-1, -1, /* -1, -1,
        -1, -1, -1, -1, */
};

static struct Timer timer[nelem(nodeidmap)];

static int sigflg = 0;
static void setflg(int sig) { sigflg = sig; }

int main(int argc, char* argv[]) {
	int ch;
	int raw = 0;
	int dotimestamps = 0;
	int timeout_ms = 1000;
	argv0 = argv[0];

	 while ((ch = getopt(argc, argv,"dhrTt:")) != -1){
		 switch (ch) {
		 case 'd': ++debug; break;
		 case 'r': ++raw; break;
		 case 'T': ++dotimestamps; break;
		 case 't': timeout_ms = atoi(optarg); break;
		 default:
			 usage();
		 }
	 }

	 argc -= optind;
	 argv += optind;

	 if (argc != 1) usage();

	 openlog(argv0, debug?LOG_PERROR:0, LOG_LOCAL2);
	 //if(!debug) setlogmask(LOG_UPTO(LOG_NOTICE));

	if(setvbuf(stdout, NULL, _IOLBF, 0))
		 syslog(LOG_WARNING, "Failed to make stdout line-buffered.");

	 if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	 if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");
	 if (signal(SIGUSR1, setflg) == SIG_ERR)  crash("signal(SIGUSR1)");

	 printf("$xoff\n");  // prevent garbage from piling up before we're done probing.
	 printf("$name epos-%s\n", basename(argv[0]));

	 int fd = epos_open(argv[0]);
	 if (fd < 0) crash("epos_open(%s)", argv[0]);

	 int found = 0;
	 int nodeid;

	 for (nodeid = 1; nodeid < nelem(nodeidmap); ++nodeid) {
		 // Read serial number register 0x1018[4]
		 uint32_t err = epos_readobject(fd, 0x1018, 4, nodeid, nodeidmap + nodeid);
		 if (err != 0) continue;
		 slog(LOG_INFO, "port:%s nodeid:%d serial:0x%x\n", argv[0], nodeid, nodeidmap[nodeid]);
		 memset(&timer[nodeid], 0, sizeof timer[0]);
		 // Ask linebusd to filter.  Note: we assume all clients will print serial in hex,
		 // which they will if they use eposclient.h EPOS_G/SET_OFMT
		 printf("$subscribe 0x%x\n", nodeidmap[nodeid]);
		 ++found;
	 }

	 printf("$xon\n");  // prevent garbage from piling up before we're done probing.

	 if(!found)
		 crash("No epos devices found");

	 while (!feof(stdin)) {

		 if(sigflg) {
			 sigflg = 0;

			 for(nodeid = 1; nodeid < nelem(nodeidmap); ++nodeid) {
				 if (nodeidmap[nodeid] == -1) continue;
				 struct TimerStats stats;
				 if(timer_stats(&timer[nodeid], &stats))
					 slog(LOG_INFO, "serial 0x%x count:%lld", nodeidmap[nodeid], stats.count);
				 else
					 slog(LOG_INFO, TIMER_OFMT(nodeidmap[nodeid], stats));
			 }
		 }

		 char line[1024];
		 if (!fgets(line, sizeof(line), stdin))
			 crash("reading stdin");

		 uint32_t serial = 0;
		 uint32_t regidx = 0;
		 char op 	 = 0;
		 int32_t value   = 0;
		 uint64_t us     = 0;

		 if(!ebus_parse_req(line, &op, &serial, &regidx, &value, &us)) {
			 slog(LOG_DEBUG, "unparseable line:\"%s\"", line);
			 continue;
		 }

		 int index       = INDEX(regidx);
		 int subindex    = SUBINDEX(regidx);

		 for(nodeid = 1; nodeid < nelem(nodeidmap); ++nodeid)
			 if (nodeidmap[nodeid] == serial)
				 break;
		 if (nodeid == nelem(nodeidmap))  // not for us
			 continue;

		 uint32_t err;		 
		 timer_tick_now(&timer[nodeid], 1);

		 switch(op) {
		 case ':':  // a set request
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
		 case '?': // a get request
			 if (raw)
				 err = epos_readobject(fd, index, subindex, nodeid, (uint32_t*)&value);
			 else
				 err = epos_waitobject(fd, timeout_ms, index, subindex, nodeid, 0, (uint32_t*)&value);
			 break;
		 }
		    
		 int64_t now = now_us();
		 if (timer_tick(&timer[nodeid], now, 0) > 100*1000) // 100ms should be enough
			 slog(LOG_WARNING, "slow epos response on serial:0x%x\n", serial);

		 if (err) { 
			 if(dotimestamps)
				 printf(EBUS_ERR_T_OFMT(serial, regidx, err, now));
			 else
				 printf(EBUS_ERR_OFMT(serial, regidx, err));
		 } else {
			 if(dotimestamps)
				 printf(EBUS_ACK_T_OFMT(serial, regidx, value, now));
			 else
				 printf(EBUS_ACK_OFMT(serial, regidx, value));

		 }
	 }

	 crash("main loop exit");
	 return 0;
}
