// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Open serial port and decode IMU MTData messages
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
		"\t-d debug            (don't syslog)\n"
		"\t-m output_mode      default 0x....\n"
		"\t-s output_settings  default 0x....\n"
		, argv0, argv0);
	exit(2);
}

 
enum {
	IMU_REQCONFIGURATION	= 0x0C, IMU_CONFIGURATION,
	IMU_GOTOMEASUREMENT	= 0x10,	IMU_GOTOMEASUREMENTACK,
	IMU_GOTOCONFIG		= 0x30,	IMU_GOTOCONFIGACK,
	IMU_MTDATA		= 0x32,
	IMU_WAKEUP		= 0x3E,	IMU_WAKEUPACK,
	IMU_ERROR		= 0x42,
};

// we use: temp, calibr.data orientation data, position data velocity data, status data, timestamp=utc time
// orientation mode = euler, datafmt = standard, send acc, send gyr, !send.mag
// mt msg len = 82  (0xFA 0xFF 0x32 0x4D ...
enum {
	// Output mode, message id 0xD0, 16 bits
	IMU_OM_TMP = 1<<0,
	IMU_OM_CAL = 1<<1,
	IMU_OM_ORI = 1<<2,
	IMU_OM_AUX = 1<<3,
	IMU_OM_POS = 1<<4,
	IMU_OM_VEL = 1<<5,
	IMU_OM_STS = 1<<11,

	// can't combine with the others, don't use
	IMU_OM_GPS = 1<<12,
	IMU_OM_RAW = 1<<14,

	// Output settings, message id 0xD2
	// timestamp: 0 is none
	IMU_OS_TS_SC = 1<<0,   // timestamp: sample counter
	IMU_OS_TS_UTC = 1<<1,  // timestamp: utc time

	// orientation output mode, 0 is quaternions.  don't mix the following
	IMU_OS_OR_EULER = 1<<2,
	IMU_OS_OR_MATRIX = 1<<3,
	IMU_OS_OR_MASK = IMU_OS_OR_EULER|IMU_OS_OR_MATRIX,

	// calibr mode disable bits
	IMU_OS_CM_DISACC = 1<<4,
	IMU_OS_CM_DISGYR = 1<<5,
	IMU_OS_CM_DISMAG = 1<<6,

	// bit 7 reserved, 8,9 float format: use 0: default.
	IMU_OS_FF_MASK = (1<<8|1<<9),
	// bit 10, 11: aux mode
	IMU_OS_AU_DIS1 = (1<<10),
	IMU_OS_AU_DIS2 = (1<<11),

	//  rest: we don't use either
};

struct IMUProto {
	int64_t timestamp_ms;
	double temp_c;
	double acc_x_m_s2;
	double acc_y_m_s2;
	double acc_z_m_s2;
	double gyr_x_rad_s;
	double gyr_y_rad_s;
	double gyr_z_rad_s;
	double roll_deg;
	double pitch_deg;
	double yaw_deg;
	double lat_deg;
	double lng_deg;
	double alt_m;
	double vel_x_m_s;
	double vel_y_m_s;
	double vel_z_m_s;
	uint8_t status;
	uint16_t samplecounter;
};

// For use in printf and friends.
#define OFMT_IMUPROTO(x, n)					 \
	"timestamp_ms:%lld temp_c:%.3lf "			 \
	"acc_x_m_s2:%.3lf acc_y_m_s2:%.3lf acc_z_m_s2:%.3lf "	 \
	"gyr_x_rad_s:%.3lf gyr_y_rad_s:%.3lf gyr_z_rad_s:%.3lf " \
	"roll_deg:%.3lf pitch_deg:%.3lf yaw_deg:%.3lf "		 \
	"lat_deg:%.6lf lng_deg:%.6lf alt_m:%.3lf "		 \
	"vel_x_m_s:%.3lf vel_y_m_s:%.3lf vel_z_m_s:%.3lf "	 \
	"%n"							 \
	, (x).timestamp_ms, (x).temp_c			 \
	, (x).acc_x_m_s2, (x).acc_y_m_s2, (x).acc_z_m_s2	\
	, (x).gyr_x_rad_s, (x).gyr_y_rad_s, (x).gyr_z_rad_s	\
	, (x).roll_deg, (x).pitch_deg, (x).yaw_deg		\
	, (x).lat_deg, (x).lng_deg, (x).alt_m		\
	, (x).vel_x_m_s, (x).vel_y_m_s, (x).vel_z_m_s	\
	, (n)


static float
decode_float(uint8_t** dd)
{
	uint8_t* d = *dd;
	uint8_t f[4] = { d[3], d[2], d[1], d[0] };
	if(debug) fprintf(stderr, "decode float: %02x %02x %02x %02x  : %lf\n", d[0],d[1],d[2],d[3], *(float*)f);
	(*dd) += 4;
	return *(float*) f;
}

// decode an MTData message assuming mode and settings.
// Returns 0 on success, -1 on error.
static int 
imu_decode_variables(uint8_t* b, int len, uint16_t mode, uint32_t settings, struct IMUProto* vars)
{
	uint8_t *e = b + len;

#define checklen(x, n)	\
	if (e - b < n) {  if (debug) fprintf(stderr, "checklen %s\n", #x); return -1; }

	if (mode & IMU_OM_TMP) {
		checklen(IMU_OM_TMP, 4);
		vars->temp_c = decode_float(&b);
	}

	if (mode & IMU_OM_CAL) {	
		if (!(settings & IMU_OS_CM_DISACC)) {
			checklen(IMU_OS_CM_DISACC, 3*4);
  			vars->acc_x_m_s2  = decode_float(&b);
			vars->acc_y_m_s2  = decode_float(&b);
			vars->acc_z_m_s2  = decode_float(&b);
		}
		if (!(settings & IMU_OS_CM_DISGYR)) {
			checklen(IMU_OS_CM_DISGYR, 3*4);
			vars->gyr_x_rad_s = decode_float(&b);
			vars->gyr_y_rad_s = decode_float(&b);
			vars->gyr_z_rad_s = decode_float(&b);
		}
		if (!(settings & IMU_OS_CM_DISMAG)) {
			checklen(IMU_OS_CM_DISMAG, 3*4);
			b += 3*4;
		}

		if (mode & IMU_OM_ORI) {
			switch (settings & IMU_OS_OR_MASK) {
			case 0: // quaternions
				checklen(IMU_OS_OR_MASK, 4*4);
				b += 4*4;
				break;
			case IMU_OS_OR_EULER:
				checklen(IMU_OS_OR_EULER, 3*4);
				vars->roll_deg  = decode_float(&b);
				vars->pitch_deg = decode_float(&b);
				vars->yaw_deg   = decode_float(&b);
				break;
			case IMU_OS_OR_MATRIX:
				checklen(IMU_OS_OR_MATRIX, 9*4);
				b += 9*4;
				break;
			}
		}
	}

	if (mode & IMU_OM_AUX) {
		if (!(settings & IMU_OS_AU_DIS1)) { checklen(IMU_OS_AU_DIS1,2); b += 2; }
		if (!(settings & IMU_OS_AU_DIS2)) { checklen(IMU_OS_AU_DIS1,2); b += 2; }
	}

	if (mode & IMU_OM_POS) { 
		checklen( IMU_OM_POS, 3*4);
		vars->lat_deg = decode_float(&b);
		vars->lng_deg = decode_float(&b);
		vars->alt_m   = decode_float(&b);
	}

	if (mode & IMU_OM_VEL) {
		checklen( IMU_OM_VEL,3*4);
		vars->vel_x_m_s = decode_float(&b);
		vars->vel_y_m_s = decode_float(&b);
		vars->vel_z_m_s = decode_float(&b);
	}

	if (mode & IMU_OM_STS) {
		checklen( IMU_OM_STS, 1);
		vars->status = *b++;
	}
		
	if (settings & IMU_OS_TS_SC) {
		checklen(IMU_OS_TS_SC, 2);
		vars->samplecounter = (b[0]<<8) + b[1]; 
		b += 2;
	}

	if (settings & IMU_OS_TS_UTC) {
		checklen( IMU_OS_TS_UTC, 12);
		if (debug || (b[11] & 0x4)) {  // valid utc
/*
  0 Nanoseconds of second, range 0 .. 1.000.000.000 
  4 Year, range 1999 .. 2099 
  6 Month, range 1..12 
  7 Day of Month, range 1..31 
  8 Hour of Day, range 0..23 
  9 Minute of Hour, range 0..59 
  10 Seconds of Minute, range 0..59 
  11 0x01 = Valid Time of Week 
  0x02 = Valid Week Number 
  0x04 = Valid UTC  
*/
			
			struct tm t = {
				b[10],	//int tm_sec;         /* seconds */
				b[9],	//int tm_min;         /* minutes */
				b[8],	//int tm_hour;        /* hours */
				b[7],	//int tm_mday;        /* day of the month */
				b[6]-1,	//int tm_mon;         /* month */
				(b[4]<<8) + b[5] - 1900, //int tm_year;        /* year */
				0,	//int tm_wday;        /* day of the week */
				0,	//int tm_yday;        /* day in the year */
				0,	//int tm_isdst;       /* daylight saving time */
			};
			if (debug) fprintf(stderr, "utc: %s\n", asctime(&t));
			int64_t time_s = timegm(&t);
			int64_t ns = (b[0]<<24) + (b[1]<<16) + (b[2]<<8) + b[3];
 			time_s *= 1000;
			ns /= 1E6;
			vars->timestamp_ms = time_s + ns;
			
		} else {
			b += 12;
		}
	}

	if (debug) fprintf(stderr, "bytes left: %d\n", e-b);

	return 0;
}

// -----------------------------------------------------------------------------
enum {
	IMU_OUTPUT_MODE = IMU_OM_TMP|IMU_OM_CAL|IMU_OM_ORI|IMU_OM_POS|IMU_OM_VEL|IMU_OM_STS,
	IMU_OUTPUT_SETTINGS = IMU_OS_TS_UTC|IMU_OS_OR_EULER,
};

int main(int argc, char* argv[]) {

	int ch;
	int baudrate = 0;

	uint16_t mode = IMU_OUTPUT_MODE;
	uint32_t settings = IMU_OUTPUT_SETTINGS;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "b:dhm:s:v")) != -1){
		switch (ch) {
		case 'b': baudrate = atoi(optarg); break;
		case 'd': ++debug; break;
		case 'v': ++verbose; break;
		case 'm':
		  mode = strtol(optarg, NULL, 0); 
		  if (errno == ERANGE)
		    crash("can't parse %s as a number\n", optarg);
		  break;
		case 's':
		  settings = strtoul(optarg, NULL, 0);
		  if (errno == ERANGE)
		    crash("can't parse %s as a number\n", optarg);
		  break;
		case 'h': 
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 1) usage();

	if (settings & IMU_OS_FF_MASK) crash("Can't decode non ieee floats");;

	setlinebuf(stdout);

	if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

	// Open serial port.
	int port = -1;
	if ((port = open(argv[0], O_RDWR | O_NOCTTY)) == -1)
		crash("open(%s, ...)", argv[0]);

	// Set serial parameters.
	{
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

	FILE* imu = fdopen(port, "r");

	int garbage = 0;
	
	while(!feof(imu)) {
		if (ferror(imu)) {
			clearerr(imu);
		}
		
 		if (garbage++ > 500)
			crash("Read only garbage from imu");

		if (fgetc(imu) != 0xfa) continue;
		if (fgetc(imu) != 0xff) continue;
		uint8_t chk = 0xff;
		uint8_t mid = fgetc(imu);  chk += mid;
		uint16_t len = fgetc(imu); chk += len;
		if (len == 255) {
			len = fgetc(imu) << 8;
			len += fgetc(imu);
			chk += len >> 8;
			chk += len;
		}
		uint8_t buf[2048];
		int i;
		for (i = 0; i < len; ++i) {
			buf[i] = fgetc(imu);
			chk += buf[i];
			if (feof(imu)) break;
			if (ferror(imu)) break;
		}
		chk += fgetc(imu);
		if (feof(imu)) break;
		if (ferror(imu)) continue;

		if (chk != 0) {
			if (debug) fprintf(stderr, "invalid checksum %0x, discarding %d bytes\n", chk, len);
			continue;
		}
		garbage = 0;

		if (mid != IMU_MTDATA) {
			if (debug) fprintf(stderr, "non MTData message, discarding %d bytes\n", len);
			continue;
		}

		struct IMUProto vars;
		memset(&vars, 0, sizeof vars);

		if (imu_decode_variables(buf, len, mode, settings, &vars) != 0) {
			if (debug) fprintf(stderr, "Could not decode MTData, discarding %d bytes\n", len);
			continue;
		}

		char line[1024];
		int n = 0;
		snprintf(line, sizeof line, OFMT_IMUPROTO(vars, &n));
		if (n > sizeof line) crash("imu proto bufer too small");
		puts(line);
	}

	crash("Terminating.");
	return 0;
}
