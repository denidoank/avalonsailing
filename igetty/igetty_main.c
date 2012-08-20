// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

// Getty for Iridium 9522B Modem that allows for sending and receiving
// SMS'es and querying Iridium time/lat/long status.
//
// usage: igetty [-f /path/to/smssend/fifo] /dev/ttyXXX
// 
// igetty initializes the modem to auto answer incoming data calls and
// to send incoming SMS'es as unsollicited status messages.  when it
// sees a CONNECT status, it will execve /sbin/login, when it sees an
// SMS message it will log it to syslog as facility uucp, priority
// notice in the form:
//
//    IRIDIUM SMS from 12345678 : "escaped sms text"
//
//  In addition, it will query the Iridium time/lat/long once a minute and log them as facility uucp,
//  priority notice in the form:
//
//    IRIDIUM GEO: timestamp_ms:12345456453 lat_deg:45.1234 lng_deg:8.54321
//
// (Syslog can be configured to forward uucp.notice messages to a
// fifo or a file that can be tailed.)
//
// igetty also creates, and opens a fifo (default /var/run/sendsms)
// where it will listen for writes of the form
// destphonenr<space>text....  destphonern should be an international
// phone number (without the + or 00 prefix), and text can be up to
// 160 characters but will be stripped of the 8th bit.
//
// When igetty has execed into a login process, writes to this fifo
// will block, so the special client program sendsms (also in this
// directory) will first select() and fail on timeout.
//
// While a data connection is active, the modem will buffer SMS'es to
// deliver them after the connection is hung up.  igetty may or may not
// have been restarted by that time.   For Avalon purposes, we may
// assume the operators won't send sms'es while dialed in.  An alternative
// scheme would be to store them in the modem and poll regularily.
//
// (we could consider reading/writing smses from/to stdin/out, but 
// in any pipeline we'd use we would have to close stdout on exec(login)
// and the parent would get the exit from the last element in the pipe)
// TODO reconsider sms i/o
//
// Documentation on the Iridium specific commands set is in the
// Iridium ISU AT command reference.

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "lib/linebuffer.h"
#include "lib/log.h"
#include "lib/timer.h"

#include "sms.h"

static const char* argv0;
static int debug = 0;

static void 
usage(void)
{
        fprintf(stderr,
                "usage: %s [options] /dev/ttyXXX\n"
                "options:\n"
                "\t-b 115200            baudrate\n"
                "\t-d                   set debug mode\n"
                "\t-f /var/run/sendsms  path to fifo to read smses to send from.\n"
                , argv0);
        exit(2);
}

static void
setserial(int port, int baudrate, char* dev)
{
        struct termios t;
        if (tcgetattr(port, &t) < 0) crash("tcgetattr(%s)", dev);

	cfmakeraw(&t);
	t.c_lflag |= ICANON;
	t.c_iflag |= IGNCR;   // ignore \r on input
	t.c_oflag  = OPOST | ONLCR;   // map \n to \r\n on output
        t.c_cflag |= CRTSCTS | CREAD;

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

        tcflush(port, TCIOFLUSH);
        if (tcsetattr(port, TCSANOW, &t) < 0) crash("tcsetattr(%s)", dev);
}

// Set the mode before execing login.
static void
setserial_final(int port, char* dev)
{
	struct termios t;
	if (tcgetattr(port, &t) < 0) crash("tcgetattr(%s)", dev);


	if (tcsetattr(port, TCSANOW, &t) < 0) crash("tcsetattr(%s)", dev);
}

static int
startswith(const char* pfx, const char* s) {
	while(*pfx)
		if (*pfx++ != *s++)
			return 0;
	return 1;
}

enum CmdState { IDLE, CGMI, CGMM, CGSN, OTHERCMD, PDU, SENDPDU, CONNECTED };
static char* statestr[] = { "IDLE", "CGMI", "CGMM", "CGSN", "OTHERCMD", "PDU", "SENDPDU", "CONNECTED" };

int
main(int argc, char* argv[])
{
	int ch;
        int baudrate = 19200;
	const char* path_to_fifo = "/var/run/sendsms";

        argv0 = strrchr(argv[0], '/');
        if (argv0) ++argv0; else argv0 = argv[0];
	
	while ((ch = getopt(argc, argv, "b:df:h")) != -1){
		switch (ch) {
		case 'b': baudrate = atoi(optarg); break;
		case 'd': ++debug; break;
		case 'f': path_to_fifo = optarg; break;
		case 'h': 
		default:
			usage();
		}
	}
	
        argv += optind; argc -= optind;

        if (argc != 1) usage();
        
        if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
        if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");

        openlog(argv0, debug?LOG_PERROR:0, LOG_UUCP);

	// create and open fifo
	if (access(path_to_fifo, F_OK) != 0 && mkfifo(path_to_fifo, 0777) != 0)
		crash("mkfifo(%s)", path_to_fifo);
	int fifo = open(path_to_fifo, O_RDWR|O_NONBLOCK|O_CLOEXEC);
	if (fifo < 0) crash("open(%s)", path_to_fifo);

	// open port
	int port = open(argv[0], O_RDWR|O_NONBLOCK);
	if (port < 0) crash("open(%s)", argv[0]);
	setserial(port, baudrate, argv[0]);

	struct LineBuffer lbout  = LB_INIT;
	struct LineBuffer lbin   = LB_INIT;
	struct LineBuffer lbfifo = LB_INIT;

	enum CmdState state = IDLE;
	
	// queue commands to initialize modem (putline adds \n, OPOST|ONLCR turns that into \r\n
	lb_putline(&lbout, "ATZ0");      // reset.  Echo should be on (default) for the main loop to keep track state
	lb_putline(&lbout, "AT+CGMI");   // query manufacturer
	lb_putline(&lbout, "AT+CGMM");   // query model
	lb_putline(&lbout, "AT+CGSN");   // query serial number
	lb_putline(&lbout, "AT+CREG=1"); // enable network registration unsolicited result code
	lb_putline(&lbout, "AT+CNMI=2,2,0,1"); // smses and status delivered spontaneously as full message, see Iridium doc section 6.12
	lb_putline(&lbout, "ATS0=1");    // answer after 1st ring

	const int64_t kQueryPeriod_us = (debug ? 10 : 60) * 1E6;
	int64_t now = now_us();
        int64_t next_query = now + kQueryPeriod_us;

	while(state != CONNECTED) {

		// write 1 line 
		while(lb_pending(&lbout) && (state == IDLE || state == SENDPDU)) {
			syslog(LOG_DEBUG, "writing (%d) line '%.*s'", lbout.eol,  lbout.eol-1, lbout.line);
			// hack; distinguish between EAGAIN this line/more lines; put in linebuffer api
			int eol = lbout.head - lbout.eol;
			int r = lb_writefd(port, &lbout);
			if (r == EAGAIN && (lbout.head - lbout.eol == eol)) continue;
			if (r == 0 || r == EAGAIN) break;
			crash("writing to modem");		
		}

		// select on fifo and on port
		fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(port, &rfds);
                FD_SET(fifo, &rfds);

                sigset_t empty_mask;
                sigemptyset(&empty_mask);

                struct timespec timeout = { 0, 0 };
                if (now < next_query) {
		  timeout.tv_nsec =  ((next_query - now) % 1000000)*1000;
		  timeout.tv_sec  =   (next_query - now) / 1000000;
		}

                int r = pselect((port>fifo?port:fifo)+1, &rfds,  NULL, NULL, &timeout, &empty_mask);
                if (r == -1 && errno != EINTR) crash("pselect");

//		syslog(LOG_DEBUG, "woke up %d", r);

                now = now_us();

		// handle port reads
		if (FD_ISSET(port, &rfds)) {
                        r = lb_readfd(&lbin, port);
                        if (r != 0 && r != EAGAIN) break;
                }

		// handle fifo reads
		if (FD_ISSET(fifo, &rfds)) {
                        r = lb_readfd(&lbfifo, fifo);
                        if (r != 0 && r != EAGAIN) break;
                }

                char line[1024];
		// handle modem lines
                while(lb_getline(line, sizeof line, &lbin) > 0) {
			if(line[0] == '\n') continue;  // ignore empty lines

			syslog(LOG_DEBUG, "modem (%s): %s", statestr[state], line);
			if (state == PDU) {
				// handle this first
			}

			if(startswith("OK", line)) {
				if (state == IDLE) syslog(LOG_DEBUG, "spurious OK");
				state = IDLE; 
				continue;
			}

			if(startswith("ERROR", line)) {
				if (state == IDLE) syslog(LOG_DEBUG, "spurious ERROR");
				else syslog(LOG_INFO, "command failed");
				//syslog(LOG_INFO, "command %s failed", lastcmd);

				state = IDLE;
				continue;
			}
			if(startswith("+CMS ERROR", line)) {
				if (state != SENDPDU) syslog(LOG_DEBUG, "spurious %s", line);
				else syslog(LOG_INFO, "command failed: %s", line + 11);
				//syslog(LOG_INFO, "command %s failed", lastcmd);
				state = IDLE;
				continue;
			}

			if(startswith("+CMGS: ", line)) {
				if (state != SENDPDU) syslog(LOG_DEBUG, "spurious SMS send status");
				state = IDLE; 
				continue;
			}

			if(startswith("AT", line)) {
				if (state != IDLE) syslog(LOG_DEBUG, "premature next command %s", line);
				// memcpy to lastcmd
				state = OTHERCMD;

				if(startswith("AT+CGMI", line)) state = CGMI;
				else if(startswith("AT+CGMM", line)) state = CGMM;
				else if(startswith("AT+CGSN", line)) state = CGSN;
				else if(startswith("AT+CMGS=", line)) state = SENDPDU;

				continue;
			}

			if(startswith("-MSGEO:", line)) {
				if (state != OTHERCMD) syslog(LOG_DEBUG, "unexpected geo report");
				int x, y, z, t;
				if (sscanf(line+7, "%d,%d,%d,%x", &x, &y, &z, &t) != 4) {
					syslog(LOG_NOTICE, "Geo report unparseable: %s", line + 7);
				} else {
					// Iridium system time epoch: March 8, 2007, 03:50:21.00 GMT. 
					// (Note: the original Iridium system time epoch was June 1, 
					// 1996, 00:00:11 GMT, and was reset to the new epoch in January, 2008).
					enum { IRIDIUM_SYSTEM_TIME_SHIFT_S = 1173325821 };
					double lat = atan2(z, sqrt(x*x + y*y)) * (180.0 / M_PI);
					double lng = atan2(y, x) * (180.0 / M_PI);
					int64_t ts_ms = t * 90LL + (IRIDIUM_SYSTEM_TIME_SHIFT_S * 1000LL);
					time_t ts_s = ts_ms / 1000;
					char buf[200];
					strftime(buf, sizeof buf, "%a %F %T", gmtime(&ts_s));
					syslog(LOG_NOTICE, "Iridium time: %s skew %lld ms", buf, (now/1000)-ts_ms);
					syslog(LOG_NOTICE, "Geo report: iridium_timestamp_ms:%lld lat_deg:%.7lf lon_deg:%.7lf", ts_ms, lat, lng);
				}
				continue;
			}

			if(startswith("+CSQ:", line)) {
				if (state != OTHERCMD) syslog(LOG_DEBUG, "unexpected quality report");
				syslog(LOG_NOTICE, "Signal quality report: %d%%", atoi(line + 5)*20);
				continue;
			}

			if(startswith("+CMT:", line)) {
//				+CMT: [<alpha>],<length><CR><LF><pdu> (PDU mode)
				state = PDU;
			}

			switch(state) {
			case CGMI: syslog(LOG_NOTICE, "Manufacturer : %s", line); break;
			case CGMM: syslog(LOG_NOTICE, "Model        : %s", line); break;
			case CGSN: syslog(LOG_NOTICE, "Serial number: %s", line); break;
			default: break;
			}

			if(startswith("CONNECT", line)) {
				state = CONNECTED;
				break;
			}
		}

		// handle fifo lines
                while(lb_getline(line, sizeof line, &lbfifo) > 0) {
			syslog(LOG_DEBUG, "fifo: %s", line);
			char *msg = strchr(line, ' ');
			if (msg) *msg++ = 0;  
			while(msg && *msg == ' ') ++msg;
			if (!msg || !*msg) {
				syslog(LOG_ERR, "ill formed sms line: %s", line);
				continue;
			}
			char *eol = msg + strlen(msg) - 1;
			while(eol > msg) {
				if (*eol != '\n' && *eol != ' ')
					break;
				*eol = 0;
				--eol;
			}
		
			unsigned char pdu[SMS_MAX_PDU_LENGTH];
			int pdu_len = EncodeSMS("", line, msg, pdu, sizeof pdu);
			if (r < 0) {
				syslog(LOG_ERR, "error encoding to PDU: %s \"%s\"", line, msg);
				continue;
			}

			// User data length equals with PDU length, except the SMSC.
			const int pdu_len_except_smsc = pdu_len - 1 - pdu[0];
			char buf[SMS_MAX_PDU_LENGTH*2+100];
			snprintf(buf, sizeof buf, "AT+CMGS=%d", pdu_len_except_smsc);
			lb_putline(&lbout, buf);

			int i;
			for (i = 0; i < pdu_len; ++i)
				sprintf(buf + i * 2, "%02X", pdu[i]);
			buf[2*pdu_len] = 0x1A;   // End PDU mode with Ctrl-Z.
			buf[2*pdu_len+1] = 0;
			lb_putline(&lbout, buf);
			syslog(LOG_INFO, "sending sms to +%s: \"%s\"", line, msg);
			syslog(LOG_DEBUG, "PDU encoded: %s",  buf);
		}

		// every minute, query signal strength, time and lat/long (response handled above)
                if (now < next_query) continue;

		while(now >= next_query) next_query += kQueryPeriod_us;

		lb_putline(&lbout, "AT-MSGEO"); // query geolocation
		lb_putline(&lbout, "AT+CSQ");   // query signal strenght

	}

	setserial_final(port, argv[0]);

	syslog(LOG_NOTICE, "executing login");
	closelog();

	// execve login 
	close(0);
	close(1);
	close(2);
	if (dup(port) != 0 || dup(port) != 1 || dup(port) != 2) crash("dupping port");
	if (setpgrp()) syslog(LOG_WARNING, "could not setpgrp: %s", strerror(errno));
	execl("/sbin/login", "/sbin/login", NULL);
	crash("execve(/sbin/login)");
	return 1;
}
