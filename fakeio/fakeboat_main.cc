// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

// Pretend to be the IMU daemon, the rudder demon and the wind sensor demon.
// Usage: plug /var/run/linebus fakeboat

#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "common/check.h"
#include "../proto/rudder.h"
#include "../proto/imu.h"
#include "../proto/wind.h"

#include "helmsman/controller_io.h"
#include "helmsman/boat_model.h"
#include "helmsman/sampling_period.h"

//static const char* version = "$Id: $";
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

static void fault(int dum) { crash("fault"); }

static void
usage(void)
{
        fprintf(stderr,
                "usage: [plug /path/to/linebus] %s\n"
                , argv0);
        exit(1);
}

static int64_t now_ms()
{
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) crash("no working clock");

        int64_t ms1 = tv.tv_sec;  ms1 *= 1000;
        int64_t ms2 = tv.tv_usec; ms2 /= 1000;
        return ms1 + ms2;
}

	
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
  if (argc != 0) usage();
  setlinebuf(stdout);
  if (!debug) openlog(argv0, LOG_PERROR, LOG_LOCAL0);
  if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
  if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");

  int nn = 0;
  int64_t last = now_ms();
  int rounds = 0:
   
  // Model
  struct RudderProto drives_reference = INIT_RUDDERPROTO;  // in
  struct RudderProto drives_actual = INIT_RUDDERPROTO;     // out, and internal input
  struct WindProto   wind_sensor = INIT_WINDPROTO;         // out
  struct IMUProto    imu = {                               // out
    last, 28.5,
    0, 0, 9.8, // acc
    0, 0, 0,   // gyr
    1, 0, 0,   // mag
    0, 0, 0,   // r/p/y
    48.2390, -4.7698, 0,   // l/l/a
    0, 0, 0,   // speed
    0, 0
  };

  ControllerInput controller_input;
  controller_input.alpha_star_rad = Deg2Rad(90);  // go East!
  Polar true_wind = Polar(Deg2Rad(180), 10);  // North wind blows South.
  DriveReferenceValuesRad drives_ref;

  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  -M_PI / 2);       // gamma_sail_ / rad, e.g. -90 degrees here 
  model.SetLatLon(48.23900, -4.7698);

  while (!feof(stdin)) {
    if (ferror(stdin)) clearerr(stdin);
    struct timespec timeout = { 0, 1E8 }; // wake up at least 10/second
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fileno(stdin), &rfds);

    sigset_t empty_mask;
    sigemptyset(&empty_mask);
    int r = pselect(fileno(stdin) + 1, &rfds, NULL, NULL, &timeout, &empty_mask);
    if (r == -1 && errno != EINTR) crash("pselect");

    if (debug>2) fprintf(stderr, "Woke up %d\n", r);

    if (FD_ISSET(fileno(stdin), &rfds)) {
      char line[1024];
      if (!fgets(line, sizeof line, stdin)) 
        crash("Reading input");
      
      if (debug) fprintf(stderr, "Got line:%s\n", line);
      
      sscanf(line, IFMT_RUDDERPROTO_CTL(&drives_reference, &nn));
    }

    int64_t now = now_ms();
    if (now < last + 100) continue;
    last = now;

    drives_ref.FromProto(drives_reference);
    model.Simulate(drives_ref, 
                   true_wind,
                   &controller_input);

    // Here come the lions, i.e. the stochastic delays by our
    // computer system, the Out-of-sync processing and communication delays by e.g.
    // wind, drive and IMU demons.
    // We can later introduce these here, for the time being we assume that they are small 
    // in comparison to our sampling period of 100ms.
    controller_input.ToProto(&wind_sensor, &drives_actual, &imu);

    if (rounds % 10 == 0)
      printf(OFMT_WINDPROTO(wind_sensor, &nn));
    if (rounds % 10 == 0)
       printf(OFMT_RUDDERPROTO_STS(drives_actual, &nn));        // actuals
    if (rounds % 4 == 0)
      printf(OFMT_IMUPROTO(imu, &nn));
    ++rounds;
  }

  crash("Terminating."); 
  return 0;
}
