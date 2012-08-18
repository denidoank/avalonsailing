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


#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "lib/linebuffer.h"
#include "lib/log.h"
#include "lib/timer.h"

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
	t.c_oflag = OPOST | ONLCR;   // map \n to \r\n on output
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

int
main(int argc, char* argv[])
{
	int ch;
        int baudrate = 115200;
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

	struct LineBuffer lbout;
	struct LineBuffer lbin;
	struct LineBuffer lbfifo;
	memset(&lbout, 0, sizeof lbout);
	memset(&lbin, 0, sizeof lbin);
	memset(&lbfifo, 0, sizeof lbfifo);

	// initialize modem (putline adds \n, OPOST|ONLCR turns that into \r\n
//	lb_putline(&lbout, "ATZ0E0");      // reset
	lb_putline(&lbout, "AT+CGMI");   // query manufacturer
#if 0
	lb_putline(&lbout, "AT+CGMM");   // query model
	lb_putline(&lbout, "AT+CGSN");   // query serial number
	lb_putline(&lbout, "AT+CREG=1"); // enable network registration unsolicited result code
#endif

	const int64_t kQueryPeriod_us = 60 * 1E6;  // once a minute
	int64_t now = now_us();
        int64_t next_query = now + kQueryPeriod_us;

	for(;;) {

		// write 1 line if buffered
		if(lb_pending(&lbout)) {
			syslog(LOG_DEBUG, "wrote line");
			int r = lb_writefd(port, &lbout);
			if (r == EAGAIN) continue;
			if (r != 0) crash("writing to modem");		
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

		syslog(LOG_DEBUG, "woke up %d", r);

                now = now_us();

		// handle port reads
		if (FD_ISSET(port, &rfds)) {
                        r = lb_readfd(&lbin, port);
                        if (r != 0 && r != EAGAIN) break;  // exit main loop if stdin no longer readable.
                }

		// handle fifo reads
		if (FD_ISSET(fifo, &rfds)) {
                        r = lb_readfd(&lbfifo, fifo);
                        if (r != 0 && r != EAGAIN) break;  // exit main loop if stdin no longer readable.
                }

                char line[1024];
		// handle modem lines
                while(lb_getline(line, sizeof line, &lbin) > 0) {
			slog(LOG_DEBUG, "modem: %s", line);
		// 	connect -> break main loop

		// 	handle unsollicited  messages 

		}

		// handle fifo lines
                while(lb_getline(line, sizeof line, &lbfifo) > 0) {
			slog(LOG_DEBUG, "fifo: %s", line);

		}

		// every minute, query signal strength, time and lat/long (response handled above)
                if (now < next_query) continue;

		while(now >= next_query) next_query += kQueryPeriod_us;

		lb_putline(&lbout, "AT-MSGEO"); // query geolocation
		lb_putline(&lbout, "AT+CSQ"); // query signal strenght

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
