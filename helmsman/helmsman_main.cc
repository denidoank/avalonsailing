// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Main loop for helmsman.
//
// on stdin read imu, wind, compass, ruddersts and helmsmanctl messages
// on stdout produce helmsmansts and rudderctl messages.
//
// When alpha_star in helmsmanctl is set to NAN, execute braking mode, otherwise
// try to sail in the direction of alpha star.   When the helmsman notices 
// a rudderctl message on the bus, it assumes someone else is doing the steering
// and it will back off sending rudderctls' for 10 seconds, but it will keep
// updating its own state and sending helmsmansts reports.
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
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>

#include "lib/linebuffer.h"
#include "lib/log.h"
#include "lib/timer.h"

#include "proto/rudder.h"
#include "proto/wind.h"
#include "proto/imu.h"
#include "proto/helmsman.h"

#include "lib/convert.h"
#include "lib/unknown.h"
#include "sampling_period.h"
#include "ship_control.h"

extern int debug;

namespace {

const char* argv0;

void usage(void) {
	fprintf(stderr,
		"usage: [plug /path/to/bus] %s [options]\n"
		"options:\n"
		"\t-d debug\n"
		, argv0);
	exit(2);
}

} // namespace

// -----------------------------------------------------------------------------
//   Main.
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {

	int ch;
	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

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

	if (argc != 0) usage();

	openlog(argv0, debug?LOG_PERROR:0, LOG_LOCAL0);
	if(!debug) setlogmask(LOG_UPTO(LOG_NOTICE));

	if(setvbuf(stdout, NULL, _IOLBF, 0))
		syslog(LOG_WARNING, "Failed to make stdout line-buffered.");

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) crash("signal");

	syslog(LOG_NOTICE, "Helmsman started");

	// name and filters for the linebusd.
	printf("$name %s\n", argv0);
	printf("$subscribe compass:\n");  // these must match the IFMT_.. prefixes
	printf("$subscribe imu:\n");
	printf("$subscribe wind:\n");
	printf("$subscribe rudder\n");   // matches rudderctl and ruddersts
	printf("$subscribe status_\n");  // matches status_{left,right,sail}
	printf("$subscribe helmctl:\n");

	struct CompassProto	compass = INIT_COMPASSPROTO;
	struct IMUProto 	imu	= INIT_IMUPROTO;
	struct WindProto	wind    = INIT_WINDPROTO;
	struct RudderProto	ruddsts = INIT_RUDDERPROTO;
	struct HelmsmanCtlProto ctl	= INIT_HELMSMANCTLPROTO;

	ControllerInput  ctrl_in;

	struct LineBuffer lbuf;
	memset(&lbuf, 0, sizeof lbuf);

	struct Timer report;   // started every printf(helsmansts)
	struct Timer backoff;  // started everytime we see a rudderctl from someone else

	memset(&report, 0, sizeof report);
	memset(&backoff, 0, sizeof backoff);

	const int64_t sampling_period_us = kSamplingPeriod * 1E6;     // .1 second * 1M us/s
	const int64_t report_period_us =  kSkipperUpdatePeriod * 1E6;  // 10 seconds * 1M/us
	const int64_t backoff_period_us = 10*1E6;  // 10 seconds

	int64_t now = now_us();
	int64_t next_run = now + sampling_period_us;
        timer_tick(&report, now, 1);

	for (;;) {

		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fileno(stdin), &rfds);

		sigset_t empty_mask;
		sigemptyset(&empty_mask);

		struct timespec timeout = { 0, 0 };
		if (now < next_run)
			timeout.tv_nsec =  (next_run - now) * 1000;

		int r = pselect(fileno(stdin)+1, &rfds,  NULL, NULL, &timeout, &empty_mask);
		if (r == -1 && errno != EINTR) crash("pselect");

		if (FD_ISSET(fileno(stdin), &rfds)) {
			r = lb_readfd(&lbuf, fileno(stdin));
			if (r != 0 && r != EAGAIN) break;  // exit main loop if stdin no longer readable.
		}

		char line[1024];
		while(lb_getline(line, sizeof line, &lbuf) > 0) {
			int nn = 0;
			if (sscanf(line, IFMT_COMPASSPROTO(&compass, &nn)) > 0) {

				ctrl_in.compass_sensor.phi_z_rad = Deg2Rad(compass.yaw_deg);

			} else if (sscanf(line, IFMT_IMUPROTO(&imu, &nn)) > 0) {

				ctrl_in.imu.Reset();
				ctrl_in.imu.FromProto(imu);

			} else if (sscanf(line, IFMT_WINDPROTO(&wind, &nn)) > 0) {

				ctrl_in.wind_sensor.Reset();
				ctrl_in.wind_sensor.alpha_deg = SymmetricDeg(NormalizeDeg(wind.angle_deg));
				ctrl_in.wind_sensor.mag_m_s   = wind.speed_m_s;
				ctrl_in.wind_sensor.valid     = wind.valid;

			} else if (sscanf(line, IFMT_RUDDERPROTO_STS(&ruddsts, &nn)) > 0) {

				ctrl_in.drives.gamma_rudder_left_rad  = Deg2Rad(ruddsts.rudder_l_deg);
				ctrl_in.drives.gamma_rudder_right_rad = Deg2Rad(ruddsts.rudder_r_deg);
				ctrl_in.drives.gamma_sail_rad         = Deg2Rad(ruddsts.sail_deg);
				ctrl_in.drives.homed_rudder_left      = !isnan(ruddsts.rudder_l_deg);
				ctrl_in.drives.homed_rudder_right     = !isnan(ruddsts.rudder_r_deg);
				ctrl_in.drives.homed_sail 	      = !isnan(ruddsts.sail_deg);

			} else if (sscanf(line, IFMT_STATUS_LEFT(&ruddsts, &nn)) > 0) {

				ctrl_in.drives.gamma_rudder_left_rad  = Deg2Rad(ruddsts.rudder_l_deg);
				ctrl_in.drives.homed_rudder_left      = !isnan(ruddsts.rudder_l_deg);

			} else if (sscanf(line, IFMT_STATUS_RIGHT(&ruddsts, &nn)) > 0) {

				ctrl_in.drives.gamma_rudder_right_rad = Deg2Rad(ruddsts.rudder_r_deg);
				ctrl_in.drives.homed_rudder_right     = !isnan(ruddsts.rudder_r_deg);

			} else if (sscanf(line, IFMT_STATUS_SAIL(&ruddsts, &nn)) > 0) {

				ctrl_in.drives.gamma_sail_rad         = Deg2Rad(ruddsts.sail_deg);
				ctrl_in.drives.homed_sail             = !isnan(ruddsts.sail_deg);

			} else if (sscanf(line, IFMT_HELMSMANCTLPROTO(&ctl, &nn)) > 0) {
				if (isnan(ctl.alpha_star_deg))
					ShipControl::Brake();
				else {
					ctrl_in.alpha_star_rad = Deg2Rad(ctl.alpha_star_deg);
					ShipControl::Normal();
				}
			} else if (strncmp(line, "rudderctl:", 10) == 0) {
				if(!timer_running(&backoff))
					syslog(LOG_WARNING, "relinquishing control");
				else 
					timer_tick(&backoff, now, 0);
				timer_tick(&backoff, now, 1);
			}
		}

		now = now_us();

		if (now < next_run) continue;

		ControllerOutput ctrl_out;
		ShipControl::Run(ctrl_in, &ctrl_out);

		while(now >= next_run) next_run += sampling_period_us;

		if (timer_running(&backoff) && timer_started(&backoff) + backoff_period_us < now) {
			syslog(LOG_WARNING, "resuming control");
			timer_tick(&backoff, now, 0);
		}

		if (!timer_running(&backoff)) {
			RudderProto ctl;
			ctrl_out.drives_reference.ToProto(&ctl);
			ctl.timestamp_ms = now / 1000;
			printf(OFMT_RUDDERPROTO_CTL(ctl));
		}

		if (now - timer_started(&report) > report_period_us) {
			timer_tick(&report, now, 0);
			timer_tick(&report, now, 1);
			ctrl_out.skipper_input.timestamp_ms = now / 1000;
			printf(OFMT_HELMSMANSTSPROTO(ctrl_out.skipper_input));
		}

	}  // for ever

	crash("Main loop exit");

	return 0;
}
