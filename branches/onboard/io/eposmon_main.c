// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Report epos communication errors
// 

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "log.h"
#include "ebus.h"
#include "actuator.h"
#include "com.h"

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------
// static const char* version = "$Id: $";
static const char* argv0;
static int verbose = 0;
static int debug = 0;

static void usage(void) {
	fprintf(stderr,
		"usage: plug -o /path/to/ebus | %s [options]\n"
		"options:\n"
		"\t-n miNimum time [s] between reports (default 10s)\n"
		"\t-x maXimum time [s] between reports (default 300s)\n"
		, argv0);
	exit(2);
}

struct Stats {
	int get, set, ack, err, fault;
} stats[4];

static const char* status_bits[] = {
	"READY",	//        "Ready to switch on",
	"ON",		//        "Switched on",
	"ENABLE",	//        "Operation enable",
	"FAULT", 	//        "Fault",
	"VOLTAGE",	//        "Voltage enabled (power stage on)",
	"QUICKSTOP",	//        "Quick stop",
	"DISABLE",	//        "Switch on disable",
	"<NOTUSED>",	//        "not used (Warning)",
	"MEASURED",	//        "Offset current measured",
	"REMOTE",	//        "Remote (NMT Slave State Operational)",
	"REACHED",	//        "Target reached",
	"LIMITED",	//        "Internal limit active",
	"ATTAINED",	//        "Set-point/ ack Speed/ Homing attained",
	"ERROR",	//        "Following error/ Not used/ Homing error",
	"REFRESH",	//        "Refresh cycle of power stage",
	"HOMEREF",	//        "Position referenced to home position",
        NULL
};

static const char* error_bits[] = {
	"GENERIC", "CURRENT", "VOLTAGE", "TEMPERATURE",
         "COMMUNICATION", "PROFILE", "RESERVED", "MOTION",
	NULL
 };

char* strbits(const char** m, uint32_t val) {
	static char buf[2048];
	char* b = buf;
	*b = 0;
	for (; *m; ++m, val>>=1)
		if (val & 1)
			b += snprintf(b, sizeof(buf)-(b-buf),"%s, ", *m);
	return buf;
}
// -----------------------------------------------------------------------------

int main(int argc, char* argv[]) {

	int ch;
	int64_t min_s = 10;
	int64_t max_s = 300;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "dhn:vx:")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
		case 'n': min_s = atoi(optarg); break;
		case 'x': max_s = atoi(optarg); break;
		case 'v': ++verbose; break;
		case 'h': 
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 0) usage();

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");

	openlog(argv0, debug?LOG_PERROR:0, LOG_LOCAL2);

	//int64_t last = now_ms();
	memset(stats, 0, sizeof stats);

	while(!feof(stdin)) {
		char line[1024];
		if (!fgets(line, sizeof(line), stdin))
			crash("reading stdin");

		 uint32_t serial = 0;
		 uint32_t regidx = 0;
		 char op 	 = 0;
		 int32_t value  = 0;
		 uint64_t us    = 0;

		 if(!ebus_parse(line, &op, &serial, &regidx, &value, &us))
			 continue;
		 
		 int i;
		 for(i = 0; i < 4; i++)
			 if(motor_params[i].serial_number == serial)
				 break;
		 
		 if(i == 4)
			 continue;  // TODO once report 'weird message'
		 
		 switch(op) {
		 case '?': stats[i].get++; break;
		 case ':': stats[i].set++; break;
		 case '=':
			 stats[i].ack++;
			 if(regidx == REG_STATUS && (value & STATUS_FAULT)) {
				 stats[i].fault++;
				 slog(LOG_ERR, "%s: Fault: %s", motor_params[i].label, strbits(status_bits, value));
			 }
			 // rudderctl asks for error on fault
			 if(regidx == REG_ERROR) {
				 slog(LOG_ERR, "%s: Error: %s", motor_params[i].label, strbits(error_bits, value));
			 }
			 break;
		 case '#':
			 stats[i].err++;
			 slog(LOG_ERR, "%s: Communication error 0x%x[%d] # 0x%x %s", motor_params[i].label, INDEX(regidx), SUBINDEX(regidx), value, epos_strerror(value));
			 break;
		 }
		 
		 // TODO report stats
		 // TODO monitor end-to-end latency
	}

	crash("main loop exit");
	return 0;
}
