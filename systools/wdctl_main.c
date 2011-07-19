// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Controller for the Linux kernel hardware watchdog.
//
// http://www.kernel.org/doc/Documentation/watchdog/watchdog-api.txt

#include <errno.h>
#include <fcntl.h>
#include <linux/watchdog.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
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
          "usage: %s [options]"
          "options:\n"
          "\t-d debug            (don't syslog)\n"
          "\t-t timeout      default 120s\n"
          "Keep system alive as long as there are new "
          "lines of data coming to stdin.\n"
          , argv0, argv0);
  exit(2);
}

int main(int argc, char* argv[]) {
  int ch;
  int timeout = 120;

  argv0 = strrchr(argv[0], '/');
  if (argv0) ++argv0; else argv0 = argv[0];

  while ((ch = getopt(argc, argv, "dht:")) != -1){
    switch (ch) {
      case 't': timeout = atoi(optarg); break;
      case 'd': ++debug; break;
      case 'h':
      default:
          usage();
    }
  }
  argv += optind;
  argc -= optind;
  if (argc != 0) usage();

  setlinebuf(stdout);
  if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

  int wdfd = open("/dev/watchdog", O_RDWR);
  if (wdfd == -1) crash("open /dev/watchdog");

  if(ioctl(wdfd, WDIOC_SETTIMEOUT, &timeout) != 0) crash("set wd timeout");

  time_t start_time = time(NULL);
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), stdin)) {
    write(wdfd, "x", 1);
  }
  long runtime = (long)(time(NULL) - start_time);
  crash("watchdog exiting after %lds - self-destruct in %d seconds...",
        runtime, timeout);
}
