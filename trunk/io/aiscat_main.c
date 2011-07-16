// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Open serial port and decode NMEA messages with AIS messages inside them.
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

#include "ais.h"

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------
// static const char* version = "$Id: $";
static const char* argv0;
static int verbose = 0;
static int debug = 0;

static void
crash(const char* fmt, ...)
{
	va_list ap;
	char buf[1000];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	if (debug)
		fprintf(stderr, "%s:%s%s%s\n", argv0, buf,
			(errno) ? ": " : "",
			(errno) ? strerror(errno):"" );
	else
		syslog(LOG_CRIT, "%s%s%s\n", buf,
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
		"usage: %s [options] [/dev/ttyXX]\n"
		"options:\n"
		"\t-b baudrate         (default unchanged)\n"
		"\t-d debug            (don't syslog, -dd opens port as plain file)\n"
		"will read NMEA sentence from stdin with no arguments.\n"
		, argv0);
	exit(2);
}


static int64_t
now_ms() 
{
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) crash("no working clock");

        int64_t ms1 = tv.tv_sec;  ms1 *= 1000;
        int64_t ms2 = tv.tv_usec; ms2 /= 1000;
        return ms1 + ms2;
}


// -----------------------------------------------------------------------------
// Return NULL if not valid, or a pointer to the end of the last field
char* valid_nmea(char* line) {
	if (line[0] != '!') return NULL;
	char* chk = strchr(line, '*');
	if (!chk) return NULL;

	char* p;
	char x = 0;
	for (p = line + 1; p < chk; p++) x ^= *p;
	char chkstr[3];
	snprintf(chkstr, sizeof chkstr, "%02X", x);
	if (debug > 2) fprintf(stderr, "checksum:%s\n", chkstr);

	if (chk[1] != chkstr[0]) return NULL;
	if (chk[2] != chkstr[1]) return NULL;

#if 0   // don't worry about trailing garbage
	if (chk[3] != '\r' && chk[3] != '\n') return NULL;
	if (chk[4] != '\n' && chk[4] != 0) return NULL;
#endif
	return chk;
}

// -----------------------------------------------------------------------------

int main(int argc, char* argv[]) {

	int ch;
	int baudrate = 0;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "b:dhv")) != -1){
		switch (ch) {
		case 'b': baudrate = atoi(optarg); break;
		case 'd': ++debug; break;
		case 'v': ++verbose; break;
		case 'h': 
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc > 1) usage();

	setlinebuf(stdout);

	if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);


	FILE* nmea = NULL;
	if (argc == 1) {
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
		nmea = fdopen(port, "r");
	} else {
		nmea = stdin;
	}
	

	int garbage = 0;

	struct aivdm_context_t ais_contexts[AIVDM_CHANNELS];

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

		if (debug  && line[0] == '#') {
			fprintf(stderr, "%s",line);
			garbage--;
			continue;
		}

		char* start = strchr(line, '!');
		if (!start){
			if (debug) fprintf(stderr, "Invalid NMEA sentence: '%s'\n", line);
			continue;
		}
		char* end = valid_nmea(start);
		if (!end) {
			if (debug) fprintf(stderr, "Invalid NMEA sentence: '%s'\n", line);
			continue;
		}

		garbage = 0;

		if (strncmp(start, "!AIVDM", 6) == 0) {
			struct ais_t ais;
			if (!aivdm_decode(start, end-start, ais_contexts, &ais)) {
				if (debug) fprintf(stderr, "Incomplete AIVDM NMEA sentence: '%s'\n", line);
				continue;
			}

			if (debug)
				fprintf(stderr, "#AIVDM:%s",line);
		  
			printf("timestamp_ms:%lld mmsi:%d msgtype:%d ", now_ms(), ais.mmsi, ais.type);

			switch(ais.type) {
			case 1: case 2: case 3:
				if (ais.type1.status != 0)
					printf("status:%d ", ais.type1.status);		/* navigation status */
				if (ais.type1.turn != AIS_TURN_NOT_AVAILABLE)
					printf("rot_deg_s:%3.1f ", ais.type1.turn *1.0);	/* rate of turn */
				if (ais.type1.speed != AIS_SPEED_NOT_AVAILABLE)
					printf("speed_m_s:%.1f ", ais.type1.speed *(1852.0/36000.0)); /* speed over ground in deciknots */
				if (ais.type1.accuracy != 0)
					printf("accuracy:%d ", ais.type1.accuracy); /* position accuracy */
				if (ais.type1.lon != AIS_LON_NOT_AVAILABLE && ais.type1.lat != AIS_LAT_NOT_AVAILABLE)
					printf("lat_deg:%.6f lng_deg:%.6f ",    /* longitude latitude */
					       ais.type1.lon / AIS_LATLON_SCALE,
					       ais.type1.lat /  AIS_LATLON_SCALE);
				if (ais.type1.course != AIS_COURSE_NOT_AVAILABLE)
					printf("cog_deg:%3.1f ", ais.type1.course*.1);  	/* course over ground */
				if (ais.type1.heading != AIS_HEADING_NOT_AVAILABLE)
					printf("heading_deg:%3.0f ",   ais.type1.heading*1.0); /* true heading */
				break;

			case 5:
				printf("size_m:%d shipname:'%s' ", ais.type5.to_bow + ais.type5.to_stern, ais.type5.shipname);
				break;

			case 18:
				printf("speed_m_s:%.1f ", ais.type18.speed *(1852.0/36000.0)); /* speed over ground in deciknots */
				if (ais.type18.accuracy != 0)
					printf("accuracy:%d ", ais.type18.accuracy); /* position accuracy */
				if (ais.type18.lon != AIS_GNS_LON_NOT_AVAILABLE && ais.type18.lat != AIS_GNS_LAT_NOT_AVAILABLE)
					printf("lat_deg:%.6f lng_deg:%.6f ",    /* longitude latitude */
					       ais.type18.lon / AIS_LATLON_SCALE,
					       ais.type18.lat /  AIS_LATLON_SCALE);
				if (ais.type18.course != AIS_COURSE_NOT_AVAILABLE)
					printf("cog_deg:%3.1f ", ais.type18.course*.1);  	/* course over ground */
				if (ais.type18.heading != AIS_HEADING_NOT_AVAILABLE)
					printf("heading_deg:%3.0f ",   ais.type18.heading*1.0); /* true heading */

				break;

			case 19:
				printf("speed_m_s:%.1f ", ais.type19.speed *(1852.0/36000.0)); /* speed over ground in deciknots */
				if (ais.type19.accuracy != 0)
					printf("accuracy:%d ", ais.type19.accuracy); /* position accuracy */
				if (ais.type19.lon != AIS_GNS_LON_NOT_AVAILABLE && ais.type19.lat != AIS_GNS_LAT_NOT_AVAILABLE)
					printf("lat_deg:%.6f lng_deg:%.6f ",    /* longitude latitude */
					       ais.type19.lon / AIS_LATLON_SCALE,
					       ais.type19.lat /  AIS_LATLON_SCALE);
				if (ais.type19.course != AIS_COURSE_NOT_AVAILABLE)
					printf("cog_deg:%3.1f ", ais.type19.course*.1);  	/* course over ground */
				if (ais.type19.heading != AIS_HEADING_NOT_AVAILABLE)
					printf("heading_deg:%3.0f ",   ais.type19.heading*1.0); /* true heading */

				printf("size_m:%d shipname:'%s' ", ais.type19.to_bow + ais.type19.to_stern, ais.type19.shipname);
				break;
				
			case 24:
				printf("size_m:%d shipname:'%s' ", ais.type24.dim.to_bow + ais.type24.dim.to_stern, ais.type24.shipname);
				break;

			}
			puts("");


			continue;
		}
		

		if (debug) fprintf(stderr, "Ignoring NMEA sentence: '%s'\n", line);
	}

	crash("Terminating.");
	return 0;
}
