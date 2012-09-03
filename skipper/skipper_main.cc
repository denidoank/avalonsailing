// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Main loop for skipper like the helmsman main.
//

#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "io2/lib/linebuffer.h"
#include "proto/helmsman.h"
#include "common/convert.h"
#include "common/now.h"
#include "helmsman/sampling_period.h"
#include "helmsman/skipper_input.h"
#include "skipper/skipper_internal.h"



extern int debug;

// -----------------------------------------------------------------------------
namespace {

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------

//const char* version = "$Id: $";
const char* argv0;
int verbose = 0;

void crash(const char* fmt, ...) {
  va_list ap;
  char buf[1000];
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  syslog(LOG_CRIT, "%s%s%s\n", buf,
	 (errno) ? ": " : "",
	 (errno) ? strerror(errno):"" );
  exit(1);
  va_end(ap);
  return;
}

static void bus_fault(int i) { crash("bus fault"); }
static void segv_fault(int i) { crash("segv fault"); }

void usage(void) {
  fprintf(stderr,
    "usage: [plug /path/to/bus] %s [options] <ais_file_path> \n"
    "options:\n"
    "\t-d debug\n"
    "\t-v verbose\n"
    , argv0);
  exit(2);
}

static const int64_t kPeriodMicros = kSamplingPeriod * 1E6;


int sscan_skipper_input(const char *line, SkipperInput* s) {
  return s->FromString(line);
}

} // namespace

// -----------------------------------------------------------------------------
//   Main.
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {

  int ch;
  argv0 = strrchr(argv[0], '/');
  if (argv0) ++argv0; else argv0 = argv[0];

  while ((ch = getopt(argc, argv, "dhv")) != -1){
    switch (ch) {
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

  openlog(argv0, debug?LOG_PERROR:0, LOG_LOCAL0);
  if(!debug) setlogmask(LOG_UPTO(LOG_NOTICE));

  if(setvbuf(stdout, NULL, _IOLBF, 0))
    syslog(LOG_WARNING, "Failed to make stdout line-buffered.");

  if (signal(SIGBUS, bus_fault) == SIG_ERR)  crash("signal(SIGBUS)");
  if (signal(SIGSEGV, segv_fault) == SIG_ERR)  crash("signal(SIGSEGV)");
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) crash("signal");

  syslog(LOG_NOTICE, "Skipper started, read AIS from %s", argv[0]);

  SkipperInput skipper_input;   // reported back from helmsman
  double alpha_star_deg;

  // wake up every 120 second
  struct timespec timeout= { 120, 0 };
  struct LineBuffer lbuf;
  memset(&lbuf, 0, sizeof lbuf);

  for (;;) {

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fileno(stdin), &rfds);

    sigset_t empty_mask;
    sigemptyset(&empty_mask);
    int r = pselect(fileno(stdin) + 1, &rfds,  NULL, NULL, &timeout, &empty_mask);
    if (r == -1 && errno != EINTR) crash("pselect");

    if (debug>2) syslog(LOG_DEBUG, "Woke up %d\n", r);

    if (r == 1) {
      r = lb_readfd(&lbuf, fileno(stdin));
      if (r == EOF) break;
      if (r == EAGAIN) continue;
      if (r != 0) crash("reading stdin");
    }

    char line[1024];
    bool new_input = false;
    while(lb_getline(line, sizeof line, &lbuf) > 0) {
      int nn = 0;
      nn = sscan_skipper_input(line, &skipper_input);
      if (nn > 0) {
        new_input = true;
      } else {
        // TODO: Fill ais.
        ;
      }
    }

    if (!new_input)
      continue;

    std::vector<skipper::AisInfo> ais;
    SkipperInternal::ReadAisFile(argv[0], &ais);
    SkipperInternal::Run(skipper_input, ais, &alpha_star_deg);

    // send desired angle to helmsman
    HelmsmanCtlProto ctl = {now_ms(), alpha_star_deg};
    if (printf(OFMT_HELMSMANCTLPROTO(ctl)) <= 0)
      crash("Could not send skipper_out_text");

    if (debug)
      fprintf(stderr,"helm: timestamp_ms:%lld alpha_star_deg:%lf\n", now_ms(), alpha_star_deg);

  }  // for ever

  crash("Main loop exit");

  return 0;
}
