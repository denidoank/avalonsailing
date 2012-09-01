// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Open serial port, decode NMEA messages and print in the various
// ../proto/*.h formats.
//
// NMEA 0183 official page:
// http://www.nmea.org/content/nmea_standards/nmea_083_v_400.asp
//
// Other unofficial and free sources:
// NMEA protocol: http://www.serialmon.com/protocols/nmea0183.html
// GPS NMEA messages: http://www.gpsinformation.org/dale/nmea.htm
// http://geoffg.net/EM408.html
//
// Currently supported:
// EM-408 GPS in default factory setup
// AIS (see aivdm.c)
// Deif WFF Windsensor
// Oceanserver OS500 digital compass in default factory setup
//
// Note the baudrates: 
//  AIS: 38400
//  Compass: 19200
//  Wind, GPS: 4800
// TODO: auto sensing, read a couple of times until valid_nmea
//
#include <ctype.h>
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

#include "proto/compass.h"
#include "proto/gps.h"
#include "proto/wind.h"

#include "lib/log.h"
#include "lib/timer.h"

#include "ais.h"

#define WIND_ANGLE_BIAS_DEG -120.0  // for windsensor rotated mount

static const char* argv0;
static int debug = 0;

static void
usage(void)
{
	fprintf(stderr,
		"usage: %s [options] [/dev/ttyXX]\n"
		"options:\n"
		"\t-a bias	add this many degrees to measured angle\n"
		"\t-b baudrate  (default 4800). Use 19200 for compass, 38400 for AIS\n"
		"\t-d debug     -dd is open serial as plain file\n"
		"\t-g seconds   default 10, use 0 to disable:if no signal for this many seconds, exit.\n"
		, argv0);
	exit(2);
}

static void
setserial(int port, int baudrate, char* dev)
{
	struct termios t;
	if (tcgetattr(port, &t) < 0) crash("tcgetattr(%s)", dev);
	cfmakeraw(&t);

	t.c_lflag |= ICANON;  // assemble to lines, but leave all echoing and special handling off.
	t.c_cflag |= CREAD;

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
	if (tcsetattr(port, TCSANOW, &t) < 0) crash("tcsetattr(%s)", dev);
}

// -----------------------------------------------------------------------------
// Return NULL if not valid, or a pointer to the '*' checksum delimiter
static char*
valid_nmea(char* line)
{
	if (line[0] != '!' && line[0] != '$') return NULL;
	char* chk = strchr(line, '*');
	if (!chk) return NULL;

	char* p;
	char x = 0;
	for (p = line + 1; p < chk; p++) x ^= *p;
	char chkstr[3];
	snprintf(chkstr, sizeof chkstr, "%02X", x);
	if (chk[1] != chkstr[0]) return 0;
	if (chk[2] != chkstr[1]) return 0;
#if 1
	if (chk[3] != '\r' && chk[3] != '\n') return NULL;
	if (chk[4] != '\n' && chk[4] != 0) return NULL;
#endif
	return chk;
}

// $WIMWV,179,R,8.0,N,A
static int
parse_wimvx(char* sentence, struct WindProto* p)
{
	char rel = 0;
	char units = 0;
	char valid = 0;
 	double speed = NAN;
	int n = sscanf(sentence, "WIMWV,%lf,%c,%lf,%c,%c,", &p->angle_deg, &rel, &speed, &units, &valid);
	if (n != 5) {
		p->angle_deg = NAN;
		speed = NAN;
		n = sscanf(sentence, "WIMWV,,%c,,%c,%c,", &rel, &units, &valid);
		if (n != 3) return 0;
	}

	p->relative = rel == 'R';
	switch(units) {
	case 'K': p->speed_m_s = speed * (1000.0/3600.0); break;// 1 km/h == 1000m / 3600s
	case 'M': p->speed_m_s = speed; break;
	case 'N': p->speed_m_s = speed * (1852.0/3600.0); break; // 1 knot == 1852m / 3600s
	default: return 0;
	}

	p->valid = valid == 'A';

	return 1;
}

// $WIXDR, C,35.2,C,2, U,28.3,N,0, U,28.6,V,1, U,3.520,V,2 *7F
static int
parse_wixdr(char* sentence, struct WixdrProto* p) {
	char dumc;
	int dumd;
	int n = sscanf(sentence, "WIXDR,%c,%lf,%c,%d,%c,%lf,%c,%d,%c,%lf,%c,%d,%c,%lf,%c,%d,", 
		       &dumc, &p->temp_c, &dumc, &dumd,
		       &dumc, &p->vheat_v, &dumc, &dumd,
		       &dumc, &p->vsupply_v, &dumc, &dumd,
		       &dumc, &p->vref_v, &dumc, &dumd);
	return n == 16;
}

static int
parse_compass(char* sentence, struct CompassProto* p)
{
	int n = sscanf(sentence, "C%lfP%lfR%lfT%lf", &p->yaw_deg, &p->pitch_deg, &p->roll_deg, &p->temp_c);
	return n == 4;
}

// $GPRMC,043356.000,A,3158.7599,S,11552.8689,E,0.24,54.42,101008,,*20
// UTC  Time  161229.487 hhmmss.sss
// Status  A  A=data valid or V=data not valid 
// Latitude   3723.2475 ddmm.mmmm
// N/S Indicator   N N=north or S=south
// Longitude  12158.3416 dddmm.mmmm
// E/W Indicator W  E=east or W=west
// Speed Over Ground  0.13  knots
// Course Over Ground  309.62  degrees True
// Date   120598  ddmmyy
// Magnetic Variation degrees E=east or W=west
static int
parse_gprmc(char* sentence, struct GPSProto *p)
{
	double time_dc = NAN;
	char status = 0;
	double lat_dc = NAN;
	char N = 0;
	double lng_dc = NAN;
	char E = 0;
	double sog_k = NAN;
	double cog_deg = NAN;
	int64_t date_dc = 0;
	int n = sscanf(sentence, "GPRMC,%lf,%c,%lf,%c,%lf,%c,%lf,%lf,%lld,",
		       &time_dc, &status, &lat_dc, &N, &lng_dc, &E, &sog_k, &cog_deg, &date_dc);
	if (n != 9) {
		sog_k = NAN;
		cog_deg = NAN;
	 	n = sscanf(sentence, "GPRMC,%lf,%c,%lf,%c,%lf,%c,,,%lld,",
		       &time_dc, &status, &lat_dc, &N, &lng_dc, &E, &date_dc);
		if (n != 7) return 0;
	}

	if (status != 'A' && status != 'V') return 0;

	if (status == 'A') {
	
		struct tm t;
		t.tm_hour = time_dc / 10000;  time_dc -= 10000 * t.tm_hour;
		t.tm_min  = time_dc / 100;    time_dc -= 100 * t.tm_min;
		t.tm_sec  = time_dc;    time_dc -=  t.tm_sec;
		int64_t ms = time_dc * 1000LL;
		t.tm_mday = date_dc / 10000; date_dc %= 10000;
		t.tm_mon = date_dc / 100; date_dc %= 100;  t.tm_mon--;
		t.tm_year = 100 + date_dc;
		p->gps_timestamp_ms = timegm(&t) * 1000LL + ms;
	
		p->lat_deg = trunc(lat_dc / 100.0);  lat_dc -= 100*p->lat_deg;  p->lat_deg += lat_dc / 60.0;
		if (N == 'S') p->lat_deg *= -1;

		p->lng_deg = trunc(lng_dc / 100.0);  lng_dc -= 100*p->lng_deg;  p->lng_deg += lng_dc / 60.0;
		if (E == 'W') p->lng_deg *= -1;
		p->speed_m_s = sog_k * (1852.0/3600.0);
		p->cog_deg = cog_deg;

	}
	return 1;
}


static struct aivdm_context_t ais_contexts[AIVDM_CHANNELS];

static int
parse_ais(const char* start, const char* end)
{
			struct ais_t ais;
			if (!aivdm_decode(start, end-start, ais_contexts, &ais)) {
				if (debug) fprintf(stderr, "Incomplete AIVDM NMEA sentence:%s", start);
				return 0;
			}

			if (debug) fprintf(stderr, "#AIVDM:%s",start);
		  
			printf("ais: timestamp_ms:%lld mmsi:%d msgtype:%d ", now_ms(), ais.mmsi, ais.type);

			switch(ais.type) {
			case 1: case 2: case 3:
				if (ais.type1.status != 0)
					printf("status:%d ", ais.type1.status);		/* navigation status */
				if (ais.type1.turn != AIS_TURN_NOT_AVAILABLE)
					printf("rot_deg_min:%d ", ais.type1.turn);	/* rate of turn in deg/minute*/
				if (ais.type1.speed != AIS_SPEED_NOT_AVAILABLE)
					printf("speed_m_s:%.1f ", ais.type1.speed *(1852.0/36000.0)); /* speed over ground in deciknots */
				if (ais.type1.accuracy != 0)
					printf("accuracy:%d ", ais.type1.accuracy); /* position accuracy */
				if (ais.type1.lon != AIS_LON_NOT_AVAILABLE && ais.type1.lat != AIS_LAT_NOT_AVAILABLE)
					printf("lat_deg:%.6f lng_deg:%.6f ",    /* longitude latitude */
					       ais.type1.lat / AIS_LATLON_SCALE,
					       ais.type1.lon /  AIS_LATLON_SCALE);
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
					       ais.type18.lat / AIS_LATLON_SCALE,
					       ais.type18.lon /  AIS_LATLON_SCALE);
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
					       ais.type19.lat / AIS_LATLON_SCALE,
					       ais.type19.lon /  AIS_LATLON_SCALE);
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
			putchar('\n');

			return 1;
		}
		


// -----------------------------------------------------------------------------

static int alarm_s = 10;
static void timeout() { crash("No valid nmea sentence for %d seconds.", alarm_s); }

int main(int argc, char* argv[]) {

	int ch;
	int baudrate = 4800; // Compass: 19200, AIS: 38400
	double wind_bias =  WIND_ANGLE_BIAS_DEG;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "a:b:dg:h")) != -1){
		switch (ch) {
		case 'a': wind_bias = atof(optarg); break;
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

	if (argc > 1) usage();
	
	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");
	if (signal(SIGALRM, timeout) == SIG_ERR)  crash("signal(SIGSALRM)");

	openlog(argv0, debug?LOG_PERROR:0, LOG_DAEMON);

	if(setvbuf(stdout, NULL, _IOLBF, 0))
		syslog(LOG_WARNING, "Failed to make stdout line-buffered.");
	
	if (alarm_s) alarm(2*alarm_s);

	FILE* nmea = stdin;
	if (argc == 1) {
		int port = -1;
		if ((port = open(argv[0], O_RDWR | O_NOCTTY)) == -1)
			crash("open(%s, ...)", argv[0]);

		// Set serial parameters.
		if (debug < 2) setserial(port, baudrate, argv[0]);

		nmea = fdopen(port, "r");
	}

	int garbage = 0;

	while(!feof(nmea)) {
		if (ferror(nmea)) {
			clearerr(nmea);
		}
		
 		if (garbage++ > 100)
			crash("Only garbage from nmea producer");

		char line[100];
		if (!fgets(line, sizeof line, nmea)) {
			if (debug) fprintf(stderr, "Read error: %s\n", strerror(errno));
			continue;
		}

		char* start = line + strcspn(line, "!$");
		char* end = valid_nmea(start);
		if (!end) {
			if (debug) fprintf(stderr, "Invalid NMEA sentence: '%s'\n", line);
			continue;
		}

		garbage = 0;
		if (alarm_s) alarm(alarm_s);

		int64_t now = now_ms();

		if (strncmp(start, "!AIVDM", 6) == 0) {
			parse_ais(start, end);
			continue;
		}

		if (strncmp(start, "$GPRMC", 6) == 0) {
			struct GPSProto vars = INIT_GPSPROTO;
			if (!parse_gprmc(start+1, &vars)) {
				if (debug) fprintf(stderr, "Invalid GPRMC sentence: '%s'\n", line);
				continue;
			}
			vars.timestamp_ms = now;
			printf(OFMT_GPSPROTO(vars));
			continue;
		}

		if (strncmp(start, "$WIMWV", 6) == 0) {
			struct WindProto vars = INIT_WINDPROTO;
			if (!parse_wimvx(start+1, &vars)) {
				if (debug) fprintf(stderr, "Invalid WIMWV sentence: '%s'\n", line);
				continue;
			}
			if (wind_bias  != 0.0) {
				vars.angle_deg += wind_bias;
				while(vars.angle_deg >= 360.0) vars.angle_deg -= 360.0;
				while(vars.angle_deg <    0.0) vars.angle_deg += 360.0;
			}
			vars.timestamp_ms = now;
                        printf(OFMT_WINDPROTO(vars));
			continue;
		}

		if (strncmp(start, "$WIXDR", 6) == 0) {
			struct WixdrProto vars = INIT_WIXDRPROTO;
			if (!parse_wixdr(start+1, &vars)) {
				if (debug) fprintf(stderr, "Invalid WIXDR sentence: '%s'\n", line);
				continue;
			}		
			vars.timestamp_ms = now;
                        printf(OFMT_WIXDRPROTO(vars));	
			continue;
		}

		if (strncmp(start, "$C", 2) == 0 && isdigit(start[2])) { 
			struct CompassProto vars = INIT_COMPASSPROTO;
			if (!parse_compass(start+1, &vars)) {
				if (debug) fprintf(stderr, "Invalid C sentence: '%s'\n", line);
				continue;
			}
			vars.timestamp_ms = now;
			printf(OFMT_COMPASSPROTO(vars));
			continue;
		}

		if (debug) fprintf(stderr, "Ignoring NMEA sentence: '%s'\n", line);
	}

	crash("Terminating.");
	return 0;
}
