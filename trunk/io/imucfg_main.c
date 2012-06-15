// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Open serial port and configure IMU 
// Cf. MTi-G User Manual and Technical Documentation, Document MT0137P, Revision H, 15 Oct 2010 
// and the MT Low-Level Communication Protocol, see mtcp.h.
// Both can be obtained from www.xsens.com
// 
// According to Low-Level Comm. Proto guide section 4.3.7, p32., and User Manual section 2.2.6, p10:
//    Marine            (17)  
//    Aerospace_nobaro  (10)
// Both use IMU, GPS and Magnetometer, but not pressure or no-slip assumption.  The user 
// manual suggests that marine is for 'significant velocities'.  XSens customer support
// mentioned marine requires to upload a magnetic declination, which we don't.
//

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "mtcp.h"

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
		syslog(LOG_ERR, "%s%s%s\n", buf,
		       (errno) ? ": " : "",
		       (errno) ? strerror(errno):"" );
	exit(1);
	va_end(ap);
	return;
}

static void fault() { crash("fault"); }

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
		"\t-c sCenario         default 17 (marine, use 10 for aerospace nobaro)\n"
		"\t-k sKipfactor       default 19\n"
		"\t-x, -y, -z  lever arm\n"
		"\t-F       issue factory reset and exit (do not set anything)\n"
		"\t-R       use plain reset to get back to measurement mode (default GoToMeasurement)\n"
		, argv0);
	exit(2);
}

 
#define MKMESSAGE(mid)        { 0xFA, 0xFF, mid, 0, (uint8_t)-(0xFF+mid) }
#define MKMESSAGE16(mid, val) { 0xFA, 0xFF, mid, 2, (val)>>8, (val), (uint8_t)-(0xFF+mid+2+((val)>>8)+(val)) }
#define MKMESSAGE32(mid, val) { 0xFA, 0xFF, mid, 4, (val)>>24, (val)>>16, (val)>>8, (val), (uint8_t)-(0xFF+mid+4+((val)>>24)+((val)>>16)+((val)>>8)+(val)) }
#define MKMESSAGE3x32(mid, val1, val2, val3) \
	{ 0xFA, 0xFF, mid, 12, \
	  (val1)>>24, (val1)>>16, (val1)>>8, (val1), \
	  (val2)>>24, (val2)>>16, (val2)>>8, (val2), \
	  (val3)>>24, (val3)>>16, (val3)>>8, (val3),	\
	  (uint8_t)-(0xFF+mid+12+ \
		     ((val1)>>24)+((val1)>>16)+((val1)>>8)+(val1)+	\
		     ((val2)>>24)+((val2)>>16)+((val2)>>8)+(val2)+	\
		     ((val3)>>24)+((val3)>>16)+((val3)>>8)+(val3))	\
	}

static const uint8_t kMsgGotoConfig[]      = MKMESSAGE(IMU_GOTOCONFIG);
static const uint8_t kMsgGotoMeasurement[] = MKMESSAGE(IMU_GOTOMEASUREMENT);
static const uint8_t kMsgFactoryReset[]    = MKMESSAGE(IMU_RESTOREFACTORYDEF);
static const uint8_t kMsgReset[]           = MKMESSAGE(IMU_RESET);

static uint32_t encode_float(float val) { return *(uint32_t*)&val; }

static float
decode_float(uint8_t** dd)
{
	uint8_t* d = *dd;
	uint8_t f[4] = { d[3], d[2], d[1], d[0] };
	if(debug>1) fprintf(stderr, "decode float: %02x %02x %02x %02x  : %lf\n", d[0],d[1],d[2],d[3], *(float*)f);
	(*dd) += 4;
	return *(float*) f;
}

static int
msg_write(int fd, const uint8_t* msg, size_t len)
{
	while (len) {
		int r = write(fd, msg, len);
		if (r < 0) return r;
		msg += r;
		len -= r;
	}
	tcdrain(fd);
	return 0;
}

static int
read_timeout(int fd, uint8_t* buf, size_t size)
{
        fd_set rfds;
        struct timeval tv = { 1, 0 };  // 1 sec
        FD_ZERO(&rfds);  FD_SET(fd, &rfds);
        int err = select(fd+1, &rfds, NULL, NULL, &tv);
        if (err == 1)
                err = read(fd, buf, size);
        return err;
}


static int
msg_read(int fd, uint8_t* msg, size_t size)
{
	static uint8_t buf[2060];
	static int p = 0;
	for (;;) {
		if (p >= sizeof buf) p = 0; // discard overflowing messages

		int r = read_timeout(fd, buf+p, sizeof(buf) - p);
		if (r <= 0) return r;
		p += r;

		if (debug>2) fprintf(stderr, "Got %d bytes, now %d\n", r, p);

		uint8_t* preamble = memchr(buf, 0xfa, p);
		if (!preamble) {
			p = 0;
			continue;
		}

		if (debug>2) fprintf(stderr, "Preamble at %d\n", preamble-buf);

		if (preamble != buf) {
			memmove(buf, preamble, preamble-buf);
			p -= preamble - buf;
		}

		if (debug>2) fprintf(stderr, "Bytes left %d\n", p);
		if (p < 4) continue; 

		int len = buf[3];
		if (len == 0xff) {
			if (p < 6) continue;
			len = (buf[4]<<8) + buf[5];
		}
		if (debug>2) fprintf(stderr, "Message len %d\n", len);
		int ll =  len + ((len>254) ? 7:5);
		if (p < ll) continue;
		uint8_t chk = 0;
		int i;
		for (i = 1; i < ll; ++i) chk += buf[i];
		if (chk != 0) {
			if (debug>2) fprintf(stderr, "Message checksum %d != 0\n", chk);
			memmove(buf, buf+ll, p-ll);
			p -= ll;
			continue;
		}
		msg[0] = buf[2];  // mid
		int s = len;
		if (s > size - 1) s = size - 1;
		memmove(msg+1, buf+ ((len>254) ? 6 : 4), s);
		memmove(buf, buf+ll, p-ll);
		p -= ll;
		return len;
	}
	return 0;
}

static int
msg_xchg(int fd, const uint8_t* w_msg, size_t w_size, uint8_t* r_msg, size_t r_size)
{
	int i;
	r_msg[0] = 0;
	for (i = 0; i < 5; ++i) {  // Retry 5 times.
		if (debug) fprintf(stderr, "Sending message 0x%x..(0x%02x)..%c\r", w_msg[2], r_msg[0], "-\\|/"[i%4]);
		tcflush(fd, TCIFLUSH);
		if (msg_write(fd, w_msg, w_size)) crash("writing %d bytes", w_size);
		int r = msg_read(fd, r_msg, sizeof r_msg);
		if (r < 0) crash("reading ack");
		if (r_msg[0] == w_msg[2] + 1) {
			if (debug) fprintf(stderr, "Sending message 0x%x..(0x%02x)..bingo!\n", w_msg[2], r_msg[0]);
			return r;  // acks' are req + 1
		}
	}
	if (debug) fprintf(stderr, "Sending message 0x%x...FAIL, got 0x%x!\n", w_msg[2], r_msg[0]);
	return -1;
}

int main(int argc, char* argv[]) {

	int ch;
	int baudrate = 115200;
	int factoryreset = 0;
	int reset = 0;
	int skipf = 19;
	int scenario = IMU_XKFSCENARIO_MARINE; 
	uint16_t mode     = IMU_OUTPUT_MODE;
	uint32_t settings = IMU_OUTPUT_SETTINGS;
	float lev_x = -15.0;
	float lev_y = -25.0;
	float lev_z =  55.0;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "c:b:dFhk:m:NRs:vx:y:z:")) != -1){
		switch (ch) {
		case 'b': baudrate = atoi(optarg); break;
		case 'c': scenario = atoi(optarg); break;
		case 'd': ++debug; break;
		case 'v': ++verbose; break;
		case 'F': ++factoryreset; break;
		case 'R': ++reset; break;
		case 'k': skipf = atoi(optarg); break;
		case 'x': lev_x = atof(optarg); break;
		case 'y': lev_y = atof(optarg); break;
		case 'z': lev_z = atof(optarg); break;
		case 'N': // toggle IMU_OS_NED bit
			settings ^= IMU_OS_NED;
			break;
		case 'm':
		  mode = strtol(optarg, NULL, 0); 
		  if (errno == ERANGE) crash("can't parse %s as a number\n", optarg);
		  break;
		case 's':
		  settings = strtoul(optarg, NULL, 0);
		  if (errno == ERANGE) crash("can't parse %s as a number\n", optarg);
		  break;
		case 'h': 
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 1) usage();

	char label[100];
	if (!debug) {	
		char* port = strrchr(argv[0], '/');  // basename of /dev/ttyXYZ
		if (port) ++port; else port = argv[0];
		snprintf(label, sizeof label, "%s(%s)", argv0, port);
		openlog(label, LOG_PERROR, LOG_DAEMON);
	}

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");

	// Open serial port.
	int port = -1;
	if ((port = open(argv[0], O_RDWR | O_NOCTTY)) == -1)
		crash("open(%s, ...)", argv[0]);

	// Set serial parameters.
	if (debug<2) {
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

	uint8_t msg[2049];
	int r;

	r = msg_xchg(port,  kMsgGotoConfig, sizeof kMsgGotoConfig, msg, sizeof msg);
	if (r < 0) crash("Unable to go to config mode.");

	if (factoryreset) {
		r = msg_xchg(port, kMsgFactoryReset, sizeof kMsgFactoryReset, msg, sizeof msg);
		if (r <= 0) crash("Unable to issue factory reset.");
		return 0;
	}

	uint8_t msgSetSkipFactor[] = MKMESSAGE16(IMU_OUTPUTSKIPFACTOR, skipf);
	r = msg_xchg(port, msgSetSkipFactor, sizeof msgSetSkipFactor, msg, sizeof msg);
	if (r < 0) crash("Unable set skipfactor");

	uint8_t msgSetOutputMode[] = MKMESSAGE16(IMU_OUTPUTMODE, mode);
	r = msg_xchg(port, msgSetOutputMode, sizeof msgSetOutputMode, msg, sizeof msg);
	if (r < 0) crash("Unable set outputmode");

	uint8_t msgSetOutputSettings[] = MKMESSAGE32(IMU_OUTPUTSETTINGS, settings);
	r = msg_xchg(port, msgSetOutputSettings, sizeof msgSetOutputSettings, msg, sizeof msg);
	if (r < 0) crash("Unable set output settings");

	uint32_t llx = encode_float(lev_x);
	uint32_t lly = encode_float(lev_y);
	uint32_t llz = encode_float(lev_z);
	uint8_t msgSetLeverArm[] = MKMESSAGE3x32(IMU_LEVERARM, llx, lly, llz);
	{ 
		uint8_t *d = msgSetLeverArm + 4;
		assert(decode_float(&d) == lev_x);
		assert(decode_float(&d) == lev_y);
		assert(decode_float(&d) == lev_z);
	}
	r = msg_xchg(port, msgSetLeverArm, sizeof msgSetLeverArm, msg, sizeof msg);
	if (r < 0) crash("Unable set lever arm");

	uint8_t msgSetScenario[] = MKMESSAGE16(IMU_SCENARIO, scenario);
	r = msg_xchg(port, msgSetScenario, sizeof msgSetScenario, msg, sizeof msg);
	if (r < 0) crash("Unable set scenario");

	if (reset) {
		r = msg_xchg(port, kMsgReset, sizeof kMsgReset, msg, sizeof msg);
		if (r < 0) crash("Unable to reset.");
	} else {
		r = msg_xchg(port,  kMsgGotoMeasurement, sizeof kMsgGotoMeasurement, msg, sizeof msg);
		if (r < 0) crash("Unable to go to measurement mode.");
	}

	syslog(LOG_INFO, "Configured IMU.");

	return 0;
}
