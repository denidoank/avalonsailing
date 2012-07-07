// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Reading NMEA data from Oceanserver OS500 digital compass
// in default factory setup.
//
// NMEA 0183 official page:
// http://www.nmea.org/content/nmea_standards/nmea_083_v_400.asp
//
// Other unofficial and free sources:
// NMEA protocol: http://www.serialmon.com/protocols/nmea0183.html
// GPS NMEA messages: http://www.gpsinformation.org/dale/nmea.htm
//
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "proto/compass.h"
#include "lib/log.h"
#include "lib/timer.h"

static const char* argv0;
static int debug = 0;

static void
usage(void)
{
	fprintf(stderr,
		"usage: %s [options] /dev/ttyXX\n"
		"options:\n"
		"\t-b baudrate (default 19200)\n"
		"\t-d debug, -dd is open argument as plain file\n"
		"\t-g seconds default 10, use 0 to disable:if no signal for this many seconds, exit.\n"
		, argv0);
	exit(2);
}

// -----------------------------------------------------------------------------
// Return NULL if not valid, or a pointer to the 2+3 sender/message type field
static char*
valid_nmea(char* line)
{
	char* start = strchr(line, '$');
	if (!start) return NULL;
	char* chk = strchr(start, '*');
	if (!chk) return NULL;

	char* p;
	char x = 0;
	for (p = start + 1; p < chk; p++) x ^= *p;
	char chkstr[3];
	snprintf(chkstr, sizeof chkstr, "%02X", x);
	if (chk[1] != chkstr[0]) return 0;
	if (chk[2] != chkstr[1]) return 0;

	if (chk[3] != '\r' && chk[3] != '\n') return NULL;
	if (chk[4] != '\n' && chk[4] != 0) return NULL;

	return start + 1;
}

static int
parse_compass(char* sentence, struct CompassProto* cp)
{
	if (sscanf(sentence, "C%lfP%lfR%lfT%lf", &cp->yaw_deg, &cp->pitch_deg, &cp->roll_deg, &cp->temp_c) == 4) {
		cp->timestamp_ms = now_ms();
		return 1;
	}
	return 0;
}

static int alarm_s = 10;
static void timeout() { crash("No valid compass signal for %d seconds.", alarm_s); }

int main(int argc, char* argv[]) {

	int ch;
	int baudrate = 19200;  //  19200, 8, 1, N

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "b:dg:h")) != -1){
		switch (ch) {
		case 'b': baudrate = atoi(optarg); break;
		case 'd': ++debug; break;
		case 'g': alarm_s = atoi(optarg); break;
		case 'h': 
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 1) usage();

	if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");
	if (signal(SIGALRM, timeout) == SIG_ERR)  crash("signal(SIGSALRM)");

	if(setvbuf(stdout, NULL, _IOLBF, 0))
		syslog(LOG_WARNING, "Failed to make stdout line-buffered.");

	if (alarm_s) alarm(2*alarm_s);

	// Open serial port.
	int port = -1;
	if ((port = open(argv[0], O_RDWR | O_NOCTTY)) == -1)
		crash("open(%s, ...)", argv[0]);

	// Set serial parameters.
	if (debug < 2) {
		struct termios t;
		if (tcgetattr(port, &t) < 0) crash("tcgetattr(%s)", argv[0]);
    
		t.c_iflag = 0;
		t.c_oflag = 0;
		t.c_lflag = 0;
		t.c_cflag |= CLOCAL|CREAD|CS8;

		switch (baudrate) {
		case 0: break;
		case 4800: cfsetspeed(&t, B4800); break;
		case 9600: cfsetspeed(&t, B9600); break;
		case 19200: cfsetspeed(&t, B19200); break;
		case 38400: cfsetspeed(&t, B38400); break;
		case 57600: cfsetspeed(&t, B57600); break;
		case 115200: cfsetspeed(&t, B115200); break;
		default: crash("Unsupported baudrate: %d", baudrate);
		}

		tcflush(port, TCIFLUSH);
		if (tcsetattr(port, TCSANOW, &t) < 0) crash("tcsetattr(%s)", argv[0]);
	}

	FILE* nmea = fdopen(port, "r");

	int garbage = 0;

	while(!feof(nmea)) {
		if (ferror(nmea)) {
			clearerr(nmea);
		}
		
		if (garbage++ > 100)
			crash("Only garbage from nmea producer");

		char line[100];
		if (!fgets(line, sizeof line, nmea)) {
			if (debug) {
				fprintf(stderr, "Error reading from port: %s\n", strerror(errno));
			}
			continue;
		}

		char* start = valid_nmea(line);
		if (!start) {
			if (debug) fprintf(stderr, "Invalid NMEA sentence: '%s'\n", line);
			continue;
		}

		garbage = 0;
		if (alarm_s) alarm(alarm_s);

		if (strncmp(start, "C", 1) == 0) {
			struct CompassProto vars = INIT_COMPASSPROTO;
			if (!parse_compass(start, &vars)) {
				if (debug) fprintf(stderr, "Invalid C sentence: '%s'\n", line);
				continue;
			}
			printf(OFMT_COMPASSPROTO(vars));
			continue;
		}

		if (debug) fprintf(stderr, "Ignoring NMEA sentence: '%s'\n", line);
	}

	crash("Terminating.");
	return 0;
}
