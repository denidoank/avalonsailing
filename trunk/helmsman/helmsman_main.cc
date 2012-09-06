// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Main loop for helmsman: open client sockets to wind_sensor and imu, server
// socket for helsman clients (e.g. the skipper and sysmon) and shovel
// data between all open file descriptors and the main controller.
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

#include "proto/gps.h"
#include "proto/rudder.h"
#include "proto/wind.h"
#include "proto/imu.h"
#include "proto/helmsman.h"
#include "proto/helmsman_status.h"
#include "proto/remote.h"
#include "skipper_input.h"

#include "common/convert.h"
#include "common/now.h"
#include "common/unknown.h"
#include "sampling_period.h"
#include "ship_control.h"

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
    "usage: [plug /path/to/bus] %s [options]\n"
    "options:\n"
    "\t-d debug\n"
    , argv0);
  exit(2);
}

static const int64_t kPeriodMicros = kSamplingPeriod * 1E6;

void AdvanceCallTime(int64_t* next_call_micros) {
  *next_call_micros += kPeriodMicros;
  if (now_micros() > *next_call_micros) {
    syslog(LOG_WARNING, "Irkss!! Too late by %lld micros\n", (now_micros() - *next_call_micros));
    *next_call_micros = now_micros();
  }
}

bool CalculateTimeOut(int64_t next_call_micros, struct timespec* timeout) {
  int64_t now = now_micros();
  int64_t until_call = next_call_micros > now ? next_call_micros - now : 0;
  if (until_call > kPeriodMicros)
    until_call = kPeriodMicros;
  CHECK(kSamplingPeriod < 1.0);
  timeout->tv_sec = 0;
  timeout->tv_nsec = 1000 * (until_call % 1000000LL);
  return until_call > 0;
}

void HandleRemoteControl(RemoteProto remote, int* control_mode) {
  if (remote.command != *control_mode)
    syslog(LOG_NOTICE, "Helmsman switched to control mode %d\n", remote.command);
  switch (remote.command) {
    case kNormalControlMode:
    case kOverrideSkipperMode:
      *control_mode = remote.command;
      ShipControl::Normal();
      break;
    case kDockingControlMode:
      *control_mode = remote.command;
       ShipControl::Docking();
      break;
    case kBrakeControlMode:
    case kPowerCycleMode:
      *control_mode = remote.command;
      ShipControl::Brake();
      break;
    case kIdleHelmsmanMode:
      *control_mode = remote.command;
      ShipControl::Idle();
      break;
  default:
    syslog(LOG_WARNING, "Illegal remote control: %d", remote.command);
  }
}

void HandleRemoteControlFailSafe(int64_t last_remote_message_millis, int* control_mode) {
  // See the alive_timer_ in remote_control/mainwindow.cc .
  const int64_t kRemoteControlTimeOutSeconds = 5;  // We get a message every 2 s and we are allowed to miss one.
  if ((kIdleHelmsmanMode == *control_mode ||
       kOverrideSkipperMode == *control_mode) &&
      now_ms() > last_remote_message_millis + kRemoteControlTimeOutSeconds * 1000) {
    *control_mode = kBrakeControlMode;
    syslog(LOG_WARNING, "helsman main: remote control communication timeout, braking");
    ShipControl::Brake();
  }
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

  if (argc != 0) usage();

  openlog(argv0, debug?LOG_PERROR:0, LOG_LOCAL0);
  if(!debug) setlogmask(LOG_UPTO(LOG_NOTICE));

  if(setvbuf(stdout, NULL, _IOLBF, 0))
    syslog(LOG_WARNING, "Failed to make stdout line-buffered.");

  if (signal(SIGBUS, bus_fault) == SIG_ERR)  crash("signal(SIGBUS)");
  if (signal(SIGSEGV, segv_fault) == SIG_ERR)  crash("signal(SIGSEGV)");
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) crash("signal");

  syslog(LOG_NOTICE, "Helmsman started");

  ControllerInput ctrl_in;
  ControllerOutput ctrl_out;  // in this scope because it keeps the statistics.

  struct WindProto wind_sensor  = INIT_WINDPROTO;
  struct CompassProto compass  = INIT_COMPASSPROTO;
  struct RudderProto sts = INIT_RUDDERPROTO;
  struct IMUProto imu    = INIT_IMUPROTO;
  struct HelmsmanCtlProto ctl = INIT_HELMSMANCTLPROTO;
  struct RemoteProto remote = INIT_REMOTEPROTO;
  struct GPSProto gps = INIT_GPSPROTO;
  ctrl_in.alpha_star_rad = Deg2Rad(225);  // Going SouthWest is a good guess (and breaks up a deadlock)
  int control_mode = kNormalControlMode;
  int64_t last_remote_message_millis = now_ms();

  int loops = 0;

  // Run ship controller exactly once every 100ms
  int64_t next_call_micros = now_micros() + kPeriodMicros;
  struct timespec timeout;
  CalculateTimeOut(next_call_micros, &timeout);

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
    while(lb_getline(line, sizeof line, &lbuf) > 0) {
      int nn = 0;
      if (sscanf(line, IFMT_WINDPROTO(&wind_sensor, &nn)) > 0) {
	ctrl_in.wind_sensor.Reset();
	ctrl_in.wind_sensor.alpha_deg = SymmetricDeg(NormalizeDeg(wind_sensor.angle_deg));
	ctrl_in.wind_sensor.mag_m_s = wind_sensor.speed_m_s;
	ctrl_in.wind_sensor.valid = wind_sensor.valid;
      } else if (sscanf(line, IFMT_IMUPROTO(&imu, &nn)) > 0) {
	ctrl_in.imu.Reset();
	ctrl_in.imu.FromProto(imu);
      } else if (sscanf(line, IFMT_RUDDERPROTO_STS(&sts, &nn)) > 0) {
	ctrl_in.drives.gamma_rudder_left_rad  = Deg2Rad(sts.rudder_l_deg);
	ctrl_in.drives.gamma_rudder_right_rad = Deg2Rad(sts.rudder_r_deg);
	ctrl_in.drives.gamma_sail_rad         = Deg2Rad(sts.sail_deg);
	ctrl_in.drives.homed_rudder_left = !isnan(sts.rudder_l_deg);
	ctrl_in.drives.homed_rudder_right = !isnan(sts.rudder_r_deg);
	ctrl_in.drives.homed_sail = !isnan(sts.sail_deg);
      } else if (sscanf(line, IFMT_STATUS_LEFT(&sts, &nn)) > 0) {
	ctrl_in.drives.gamma_rudder_left_rad  = Deg2Rad(sts.rudder_l_deg);
	ctrl_in.drives.homed_rudder_left = !isnan(sts.rudder_l_deg);
      } else if (sscanf(line, IFMT_STATUS_RIGHT(&sts, &nn)) > 0) {
	ctrl_in.drives.gamma_rudder_right_rad  = Deg2Rad(sts.rudder_r_deg);
	ctrl_in.drives.homed_rudder_right = !isnan(sts.rudder_r_deg);
      } else if (sscanf(line, IFMT_STATUS_SAIL(&sts, &nn)) > 0) {
	ctrl_in.drives.gamma_sail_rad  = Deg2Rad(sts.sail_deg);
	ctrl_in.drives.homed_sail = !isnan(sts.sail_deg);
      } else if (sscanf(line, IFMT_COMPASSPROTO(&compass, &nn)) > 0) {
	ctrl_in.compass_sensor.phi_z_rad  = Deg2Rad(compass.yaw_deg);
      } else if (sscanf(line, IFMT_GPSPROTO(&gps, &nn)) > 0) {
        ctrl_in.gps.latitude_deg = gps.lat_deg;
        ctrl_in.gps.longitude_deg = gps.lng_deg;
        ctrl_in.gps.speed_m_s = gps.speed_m_s;
	    ctrl_in.gps.cog_rad = Deg2Rad(gps.cog_deg);
      } else if (sscanf(line, IFMT_HELMSMANCTLPROTO(&ctl, &nn)) > 0) {
	    if (control_mode != kOverrideSkipperMode &&
	        !isnan(ctl.alpha_star_deg))
	      ctrl_in.alpha_star_rad = Deg2Rad(ctl.alpha_star_deg);
      } else if (sscanf(line, IFMT_REMOTEPROTO(&remote, &nn)) > 0) {
    HandleRemoteControl(remote, &control_mode);
    last_remote_message_millis = now_ms();
	if (control_mode == kOverrideSkipperMode &&
	    !isnan(remote.alpha_star_deg))
	  ctrl_in.alpha_star_rad = Deg2Rad(remote.alpha_star_deg);
      } else {
  // For debugging only:    
	// Any unexpected input (messages not sent to us, or debug output that
	// accidentally was sent to stdout instead of stderr comes here.
	// syslog(LOG_DEBUG, "Unreadable input \n>>>%s<<<\n", line);
      }
    }

    HandleRemoteControlFailSafe(last_remote_message_millis, &control_mode);

    if (!CalculateTimeOut(next_call_micros, &timeout)) {
      ctrl_out.Reset();
      ShipControl::Run(ctrl_in, &ctrl_out);
      AdvanceCallTime(&next_call_micros);

      if (!ShipControl::Idling()) {
        RudderProto ctl;
        ctrl_out.drives_reference.ToProto(&ctl);
        ctl.timestamp_ms = now_ms();
        printf(OFMT_RUDDERPROTO_CTL(ctl));
      }
      // One minute should be enough to execute the last direction change.
      if (loops % static_cast<int>(60.0 / kSamplingPeriod) == 0) {
        SkipperInput to_skipper(
            now_ms(),
            ctrl_out.skipper_input.latitude_deg,
            ctrl_out.skipper_input.longitude_deg,
            ctrl_out.skipper_input.angle_true_deg,
            ctrl_out.skipper_input.mag_true_kn
        );
        fprintf(stderr, "Checking lat lon %lf %lf \n",ctrl_out.skipper_input.latitude_deg,  ctrl_out.skipper_input.longitude_deg);
        if (to_skipper.Valid()) {
          printf("%s", to_skipper.ToString().c_str());
        }
      }
      if (loops % 20 == 5) {
        HelmsmanStatusProto hsts = INIT_HELMSMAN_STATUSPROTO;
        ctrl_out.status.ToProto(&hsts);
        hsts.timestamp_ms = now_ms();
        printf(OFMT_HELMSMAN_STATUSPROTO(hsts));
      }

      ++loops;
      loops %= 1000;
    }
  }  // for ever

  crash("Main loop exit");

  return 0;
}
