// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Control power switches using a Advantech ADAM-4068 relay control module
// See ftp.bb-elec.com/bb-elec/literature/manuals/Advantech/ADAM-4000.pdf
// for more detailed documentation and command reference.
//
// The device is assumed to be in default settings (after INIT* state)
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
          "usage: %s [options] /dev/ttyXX circuit=on|off ...\n"
          "circuits: epos, sensors, cpu\n"
          "options:\n"
          "\t-d debug            (don't syslog)\n"
          , argv0);
  exit(2);
}

int relay_set(FILE *adam, int port, int op) {
  // #AAPPDD<CR> command to set output of port PP to DD on module AA.
  // Single port addressing format is 1<p>, where p is the output port number.
  // Address hard-coded to module '01'.
  fprintf(adam, "#011%d0%d\r", port, op);

  struct pollfd pfd;
  pfd.fd = fileno(adam);
  pfd.events = POLLIN;
  if (poll(&pfd, 1, 250) != 1) return 1; // timeout or error

  // expect ><CR> on success
  char buffer[30];
  if (read(fileno(adam), buffer, sizeof(buffer)) == 2 &&
      strncmp(">\r", buffer, 2) == 0) return 0;
  else return 1;
}

int main(int argc, char* argv[]) {
  argv0 = strrchr(argv[0], '/');
  if (argv0) ++argv0; else argv0 = argv[0];
  int ch;
  while ((ch = getopt(argc, argv, "dh")) != -1){
    switch (ch) {
      case 'd': ++debug; break;
      case 'h':
      default:
          usage();
    }
  }

  argv += optind;
  argc -= optind;

  if (argc < 2) usage();

  setlinebuf(stdout);

  if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

  // Open serial port.
  int port = -1;
  if ((port = open(argv[0], O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1)
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

    cfsetspeed(&t, B9600);

    tcflush(port, TCIFLUSH);
    if (tcsetattr(port, TCSANOW, &t) < 0) crash("tcsetattr(%s)", argv[0]);
  }
  FILE* adam = fdopen(port, "w");
  if (setvbuf(adam, NULL, _IONBF, 0) != 0) crash("set unbuffered write");

  int rc = 0;
  int i;
  for (i = 1; i < argc; i++) {
    char *circuit = strtok(argv[i], "=");
    char *operation = strtok(NULL, "=");

    if (circuit == NULL || operation == NULL)crash("invalid command: %s", argv[i]);
    if (strcmp(operation, "on") != 0 && strcmp(operation, "off") != 0)
      crash("operation must be 'on' or 'off'");

    int op_code = strcmp(operation, "off") == 0 ? 1 : 0;

    if (strcmp(circuit, "cpu") == 0) rc |= relay_set(adam, 0, op_code);
    else if (strcmp(circuit, "epos") == 0) rc |= relay_set(adam, 1, op_code);
    else if (strcmp(circuit, "sensors") == 0) rc |= relay_set(adam, 2, op_code);
    else crash("unknown circuit %s", circuit);
  }
  return rc;
}
