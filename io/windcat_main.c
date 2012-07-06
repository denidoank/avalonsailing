// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Open serial port and decode NMEA messages
// 
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
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#include "../proto/wind.h"

#include "log.h"
#include "timer.h"

#define DEFAULT_BIAS_DEG -120.0

static const char* argv0;
static int debug = 0;

static void
usage(void)
{
	fprintf(stderr,
		"usage: %s [options] /dev/ttyXX\n"
		"options:\n"
		"\t-a bias	add this many degrees to measured angle\n"
		"\t-b baudrate  (default 4800)\n"
		"\t-d debug     -dd is open serial as plain file\n"
		"\t-g seconds   default 10, use 0 to disable:if no signal for this many seconds, exit.\n"
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

// $WIMWV,179,R,8.0,N,A
int parse_wimvx(char* sentence, struct WindProto* wp) {
	char rel = 0;
	char units = 0;
	char valid = 0;
 	double speed = NAN;
	int n = sscanf(sentence, "WIMWV,%lf,%c,%lf,%c,%c,", &wp->angle_deg, &rel, &speed, &units, &valid);
	if (n != 5) {
		wp->angle_deg = NAN;
		speed = NAN;
		n = sscanf(sentence, "WIMWV,,%c,,%c,%c,", &rel, &units, &valid);
		if (n != 3) return 0;
	}

	wp->relative = rel == 'R';
	switch(units) {
	case 'K': wp->speed_m_s = speed * (1000.0/3600.0); break;// 1 km/h == 1000m / 3600s
	case 'M': wp->speed_m_s = speed; break;
	case 'N': wp->speed_m_s = speed * (1852.0/3600.0); break; // 1 knot == 1852m / 3600s
	default: return 0;
	}

	wp->valid = valid == 'A';

	return 1;
}

// $WIXDR, C,35.2,C,2, U,28.3,N,0, U,28.6,V,1, U,3.520,V,2 *7F
int parse_wixdr(char* sentence, struct WixdrProto* wp) {
	char dumc;
	int dumd;
	int n = sscanf(sentence, "WIXDR,%c,%lf,%c,%d,%c,%lf,%c,%d,%c,%lf,%c,%d,%c,%lf,%c,%d,", 
		       &dumc, &wp->temp_c, &dumc, &dumd,
		       &dumc, &wp->vheat_v, &dumc, &dumd,
		       &dumc, &wp->vsupply_v, &dumc, &dumd,
		       &dumc, &wp->vref_v, &dumc, &dumd);
	return n == 16;
}


static int alarm_s = 10;
static void timeout() { crash("No valid wind signal for %d seconds.", alarm_s); }

// -----------------------------------------------------------------------------

int main(int argc, char* argv[]) {

	int ch;
	int baudrate = 4800;
	double bias_deg = DEFAULT_BIAS_DEG;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "a:b:dg:h")) != -1){
		switch (ch) {
		case 'a': bias_deg = strtod(optarg, NULL); break;
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
	
	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");
	if (signal(SIGALRM, timeout) == SIG_ERR)  crash("signal(SIGSALRM)");

	openlog(argv0, debug?LOG_PERROR:0, LOG_DAEMON);

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
		cfmakeraw(&t);

		t.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
		t.c_oflag &= ~OPOST;
		t.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
		t.c_cflag &= ~(CSIZE | PARENB);
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
			if (debug) fprintf(stderr, "Error reading from port: %s\n", strerror(errno));
			continue;
		}

		char* start = valid_nmea(line);
		if (!start) {
			if (debug) fprintf(stderr, "Invalid NMEA sentence: '%s'\n", line);
			continue;
		}

		garbage = 0;
		if (alarm_s) alarm(alarm_s);

		if (strncmp(start, "WIMWV", 5) == 0) {
			struct WindProto vars = { now_ms(), 0, 0, 0 };
			if (!parse_wimvx(start, &vars)) {
				if (debug) fprintf(stderr, "Invalid WIMWV sentence: '%s'\n", line);
				continue;
			}
			if (bias_deg != 0.0) {
				vars.angle_deg += bias_deg;
				while(vars.angle_deg >= 360.0) vars.angle_deg -= 360.0;
				while(vars.angle_deg <    0.0) vars.angle_deg += 360.0;
			}	
                        printf(OFMT_WINDPROTO(vars));
			continue;
		}

		if (strncmp(start, "WIXDR", 5) == 0) {
			struct WixdrProto vars = { now_ms(), 0, 0, 0, 0 };
			if (!parse_wixdr(start, &vars)) {
				if (debug) fprintf(stderr, "Invalid WIMWV sentence: '%s'\n", line);
				continue;
			}		
                        printf(OFMT_WIXDRPROTO(vars));	
			continue;
		}

		if (debug) fprintf(stderr, "Ignoring NMEA sentence: '%s'\n", line);  // TODO remove or improve voltage logging
	}

	crash("Terminating.");
	return 0;
}
