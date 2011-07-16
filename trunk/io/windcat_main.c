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
#include <sys/time.h>
#include <unistd.h>

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
		"usage: %s [options] /dev/ttyXX\n"
		"options:\n"
		"\t-b baudrate         (default unchanged)\n"
		"\t-d debug            (don't syslog, -dd is open serial as plain file)\n"
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
// Return NULL if not valid, or a pointer to the 2+3 sender/message type field
char* valid_nmea(char* line) {
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

struct WindProto {
	uint64_t timestamp_ms;
	double angle_deg;
	int relative;
	double speed_m_s;
	int valid;
};

// For use in printf and friends.
#define OFMT_WINDPROTO(x, n) \
	"timestamp_ms:%lld angle_deg:%.3lf speed_m_s:%.2lf valid:%d%n", \
	(x).timestamp_ms, (x).angle_deg, (x).speed_m_s, (x).valid, (n)

int parse_wimvx(char* sentence, struct WindProto* wp) {
	char* flde;
	char* fld = strsep(&sentence, ",");   // field 0: sender and message type
	if (!fld) return 0;

	fld = strsep(&sentence, ",");   // field 1: angle in degrees
	if (!fld) return 0;
	wp->angle_deg = strtod(fld, &flde);
	if (*flde) return 0;

	fld = strsep(&sentence, ",");  // field 2: "R" for relative
	if (!fld) return 0;
	wp->relative = (*fld == 'R');

	fld = strsep(&sentence, ",");  // field 3: speed
	if (!fld) return 0;
	double speed = strtod(fld, &flde);
	if (*flde) return 0;

	fld = strsep(&sentence, ",");  // field 4: units
	if (!fld) return 0;
	switch(fld[0]) {
	case 'K': wp->speed_m_s = speed * (1000.0/3600.0); break;// 1 km/h == 1000m / 3600s
	case 'M': wp->speed_m_s = speed; break;
	case 'N': wp->speed_m_s = speed * (1852.0/3600.0); break; // 1 knot == 1852m / 3600s
	default: return 0;
	}

	fld = strsep(&sentence, ",");  // field 5: 'A' for valid
	if (!fld) return 0;

	wp->valid = *fld == 'A';

	return 1;
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

	if (argc != 1) usage();

	setlinebuf(stdout);

	if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

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
	char out[1024];

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

		if (strncmp(start, "WIMWV", 5) == 0) {
			struct WindProto vars = { now_ms(), 0, 0, 0 };
			if (!parse_wimvx(start, &vars)) {
				if (debug) fprintf(stderr, "Invalid WIMWV sentence: '%s'\n", line);
				continue;
			}
			int n = 0;
			snprintf(out, sizeof out, OFMT_WINDPROTO(vars, &n));
			if (n > sizeof out) crash("wind proto bufer too small");
			puts(out);
			continue;
		}

		if (strncmp(start, "WIXDR", 5) == 0) {
			// ignore: temperature and voltages
			continue;
		}

		if (debug) fprintf(stderr, "Ignoring NMEA sentence: '%s'\n", line);
	}

	crash("Terminating.");
	return 0;
}
