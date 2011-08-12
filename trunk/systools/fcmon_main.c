// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Poll fuel-cell control port for status info and print to stdout.
//

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "proto/fuelcell.h"

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------
// static const char* version = "$Id: $";
static const char* argv0;
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
          "\t-d debug            (don't syslog)\n"
          "\t-p period           (60s)\n"
          , argv0);
  exit(2);
}

int fc_status(FILE *fc) {
  fprintf(fc, "sfc\r");

  char buffer[256];
  int found_mask = 0;
  struct pollfd pfd;

  struct FuelcellProto fc_status = INIT_FUELCELLPROTO;

  while (1) {
    pfd.fd = fileno(fc);
    pfd.events = POLLIN;
    if (poll(&pfd, 1, 5000) != 1) return 1; // timeout or error

    if (fscanf(fc, "%[^\r]\r", buffer) != 1) return 1; // error

    if (sscanf(buffer, "battery voltage %lfV", &fc_status.tension_V) == 1) {
      found_mask |= 1;
    } else if (sscanf(buffer, "output current %lfA",
                      &fc_status.charge_current_A) == 1) {
      found_mask |= 1 << 1;
    } else if (sscanf(buffer, "cumulative output energy %lfWh",
               &fc_status.energy_Wh) == 1) {
      found_mask |= 1 << 2;
    } else if (sscanf(buffer, "operation time (charge mode) %lfh",
               &fc_status.runtime_h) == 1) {
      found_mask |= 1 << 3;
    }

    if (debug > 1) fprintf(stderr, "'%s'\n", buffer);

    if (found_mask == 15) {
      int n;
      printf(OFMT_FUELCELLPROTO(fc_status, &n));

      while (1) { // clean up input until there is nothing left
        pfd.fd = fileno(fc);
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 100) != 1) return 0;
        fgetc(fc);
      }
      return 0;
    }
  }
}

int main(int argc, char* argv[]) {
  argv0 = strrchr(argv[0], '/');
  if (argv0) ++argv0; else argv0 = argv[0];
  int ch;
  int period = 60;
  while ((ch = getopt(argc, argv, "dp:h")) != -1){
    switch (ch) {
      case 'd': ++debug; break;
      case 'p': period = atoi(optarg); break;
      case 'h':
      default:
          usage();
    }
  }

  argv += optind;
  argc -= optind;

  if (argc < 1) usage();

  setlinebuf(stdout);

  if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

  // Open serial port.
  int port = -1;
  if ((port = open(argv[0], O_RDWR | O_NOCTTY)) == -1)
    crash("open(%s, ...)", argv[0]);

  // Set serial parameters.
  if (debug < 2)
  {
    struct termios t;
    if (tcgetattr(port, &t) < 0) crash("tcgetattr(%s)", argv[0]);
    cfmakeraw(&t);

    t.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR |
                   IGNCR | ICRNL | IXON);
    t.c_oflag &= ~OPOST;
    t.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t.c_cflag &= ~(CSIZE | PARENB);
    t.c_cflag |= CLOCAL|CREAD|CS8;

    cfsetspeed(&t, B9600);

    tcflush(port, TCIFLUSH);
    if (tcsetattr(port, TCSANOW, &t) < 0) crash("tcsetattr(%s)", argv[0]);
  }
  FILE* fc = fdopen(port, "r+");
  if (fc == NULL) crash("fdopen");
  if (setvbuf(fc, NULL, _IONBF, 0) != 0) crash("set unbuffered write");

  while (1) {
    if (fc_status(fc) != 0 && debug > 0) crash("fc_status");
    sleep(period);
  }
}
