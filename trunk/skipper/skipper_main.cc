// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Main loop for skpper: open client sockets to aisd and helmsmand, and shovel
// data between all open file descriptors and the main controller.
//

#include <vector>

#include <errno.h>
#include <fcntl.h>
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
#include <sys/un.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "common/convert.h"
#include "common/unknown.h"
#include "helmsman/skipper_input.h"
#include "skipper/skipper_internal.h"

// -----------------------------------------------------------------------------
namespace {

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------

const char* version = "$Id: $";
const char* argv0;
int verbose = 0;
int debug = 0;

void crash(const char* fmt, ...) {
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

void usage(void) {
  fprintf(stderr,
         "usage: %s [options]\n"
         "options:\n"
         "\t-C /working/dir     (default /var/run)\n"
         "\t-S /path/to/socket  (default /working/dir/%s\n"
         "\t-W /path/to/wind    (default /working/dir/wind\n"
         "\t-I /path/to/imud    (default /working/dir/imud\n"
         "\t-R /path/to/rudderd (default /working/dir/rudderd\n"
         "\t-f forward wind/imu/rudderd messages to clients\n"
         "\t-d debug (don't go daemon, don't syslog)\n"
         , argv0, argv0);
  exit(2);
}


int clsockopen(const char* path) {
  int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
  if (fd < 0) crash("socket");
  struct sockaddr_un addr = { AF_LOCAL, 0 };
  strncpy(addr.sun_path, path, sizeof(addr.sun_path));
  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(fd);
    return 0;
  }
  if (fcntl(fd,  F_SETFL, O_NONBLOCK) < 0) crash("fcntl(%s)", path);
  return fd;
}

int clsockopen_wait(const char* path) {
  int s;
  for (int i = 0; ; i++) {
    s = clsockopen(path);
    if (s) break;
    fprintf(stderr, "Waiting for %s...%c\r", path, "-\\|/"[i++%4]);
    sleep(1);
  }
  fprintf(stderr, "Waiting for %s...bingo!\n", path);
  return s;
}

void set_fd(fd_set* s, int* maxfd, FILE* f) {
        int fd = fileno(f);
        FD_SET(fd, s);
        if (*maxfd < fd) *maxfd = fd;
}

// -----------------------------------------------------------------------------
//   I/O parsing and printing
// All return false on garbage
// -----------------------------------------------------------------------------

// See helmsman_main.cc::snprint_helmsmanout
int sscan_skipper_input(const char *line, SkipperInput* s) {
  s->Reset();
  while (*line) {
    char key[16];
    double value;

    int skip = 0;
    int n = sscanf(line, "%16[a-z_]:%lf %n", key, &value, &skip);
    if (n < 2) return 0;
    if (skip == 0) return 0;
    line += skip;

    if (!strcmp(key, "lat_deg")        && !isnan(value)) { s->longitude_deg  = value; continue; }
    if (!strcmp(key, "lng_deg")        && !isnan(value)) { s->latitude_deg   = value; continue; }
    if (!strcmp(key, "wind_angle_deg") && !isnan(value)) { s->angle_true_deg = value; continue; }
    if (!strcmp(key, "wind_speed_kn")  && !isnan(value)) { s->mag_true_kn    = value; continue; }
    if (!strcmp(key, "timestamp_ms"))  continue;

    return 0;
  }
  return 1;
}


uint64_t now_ms() {
  timeval tv;
  if (gettimeofday(&tv, NULL) < 0) crash("gettimeofday");
  uint64_t ms1 = tv.tv_sec;  ms1 *= 1000;
  uint64_t ms2 = tv.tv_usec; ms2 /= 1000;
  return ms1 + ms2;
}

// -----------------------------------------------------------------------------
} // namespace


// -----------------------------------------------------------------------------
//   Main.
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {

  const char* cwd = "/var/run";

  const char* path_to_aisd = "aisd";
  const char* path_to_helmsmand = "helmsmand";

  int ch;
  int forward = 0;
  int skipper_update_downsample = 0;

  argv0 = strrchr(argv[0], '/');
  if (argv0) ++argv0; else argv0 = argv[0];

  while ((ch = getopt(argc, argv, "A:C:H:dfhv")) != -1){
      switch (ch) {
      case 'C': cwd = optarg; break;
      case 'A': path_to_aisd    = optarg; break;
      case 'H': path_to_helmsmand = optarg; break;
      case 'd': ++debug; break;
      case 'f': ++forward; break;
      case 'v': ++verbose; break;
      case 'h':
      default:
         usage();
      }
  }

  if (argc != optind) usage();

  if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

  if (strcmp(cwd, ".") && chdir(cwd) < 0) crash("chdir(%s)", cwd);
  cwd = getcwd(NULL, 0);

  // Open client sockets.  Wait until they're all there so you can start up in any order.
  FILE* aisd = fdopen(clsockopen_wait(path_to_aisd), "r");
  FILE* held = fdopen(clsockopen_wait(path_to_helmsmand), "r+");
  setlinebuf(held);

  if (debug)
    fprintf(stderr, "Skipper version %s working dir %s", version, cwd);
  else
    syslog(LOG_INFO, "Skipper version %s working dir %s", version, cwd);

  // Go daemon and write pidfile.
  if (!debug) {
    daemon(0,0);

    char* path_to_pidfile = NULL;
    asprintf(&path_to_pidfile, "%s.pid", argv0);
    FILE* pidfile = fopen(path_to_pidfile, "w");
    if(!pidfile) crash("writing pidfile");
    fprintf(pidfile, "%d\n", getpid());
    fclose(pidfile);
    free(path_to_pidfile);
  }

  bool skipper_out_pending = false;
  SkipperInput skipper_input;   // reported back from helmsman
  std::vector<AISInfo> ais;
  double alpha_star_deg;

  // Main loop
  for (;;) {
    // wake up every 120 second
    timespec timeout = { 120, 0 };
    fd_set rfds;
    fd_set wfds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    int max_fd = -1;

    //    set_fd(&rfds, &max_fd, aisd);
    set_fd(&rfds, &max_fd, held);

    if (skipper_out_pending) set_fd(&wfds, &max_fd, held);

    sigset_t empty_mask;
    sigemptyset(&empty_mask);
    int r = pselect(max_fd + 1, &rfds, &wfds, NULL, &timeout, &empty_mask);
    if (r == -1 && errno != EINTR) crash("pselect");

    if (FD_ISSET(fileno(held), &wfds)) {
      fflush(held);
      skipper_out_pending = false;
    }

    if (FD_ISSET(fileno(held), &rfds)) {
      char line[1024];
      while (fgets(line, sizeof line, held)) {
	int n = sscan_skipper_input(line, &skipper_input);
	if (!n && debug) fprintf(stderr, "Ignoring garbage from helmsmand: %s\n", line);
      }
      if (feof(held) || (ferror(held) && errno!= EAGAIN)) crash("reading from helmsmand");
    }

    SkipperInternal::Run(skipper_input, ais, &alpha_star_deg);

    // send desired angle to helmsman
    if (fprintf(held,"timestamp_ms:%lld alpha_star_deg:%lf\n", now_ms(), alpha_star_deg) <= 0)
      crash("Could not send skipper_out_text");
    skipper_out_pending = true;

    if (debug) 
      fprintf(stderr,"timestamp_ms:%lld alpha_star_deg:%lf\n", now_ms(), alpha_star_deg);
    
  }  // for ever

  crash("Main loop exit");

  return 0;
}
