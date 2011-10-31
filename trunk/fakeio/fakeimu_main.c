// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

// Pretend to be the IMU daemon
// Usage: plug /var/run/linebus fakeimu

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

static double rad(double deg) { return deg * M_PI/180.0; }
static double deg(double rad) { return rad * 180.0/M_PI; }
static double sqr(double x) { return x*x; }
static int sign(double x) { return x < 0 ? -1 : 1; }
static void cap(double* x, double limit) {
  if (*x > limit)
    *x = limit;
  if (*x < -limit)
    *x = -limit;
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
	struct RudderProto rudd = INIT_RUDDERPROTO;
	struct WindProto   wind = INIT_WINDPROTO;

	struct IMUProto    imu = {
		last, 28.5,
		0, 0, 9.8,   // acc
		0, 0, 0,   // gyr
		1, 0, 0,   // mag
		0, 0, 0,   // r/p/y
		48.2390, -4.7698, 0,   // l/l/a
		0, 0, 0,   // speed
		0, 0
	};

	while (!feof(stdin)) {

		if (ferror(stdin)) clearerr(stdin);
    struct timespec timeout = { 0, 2E8 }; // wake up at least 5/second
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
			
			sscanf(line, IFMT_RUDDERPROTO_STS(&rudd, &nn));
			sscanf(line, IFMT_WINDPROTO(&wind, &nn));
		}

		int64_t now = now_ms();
			
		if (!isnan(rudd.sail_deg) && !isnan(wind.angle_deg)) {
                        // A parody of a boat model.
			const double kCWind = 1.0;
			double wind_cross_m_s = wind.speed_m_s*sin(rad(wind.angle_deg));
			double f_sail = kCWind * sqr(wind_cross_m_s);
			
			const double kDrag_x = 0.1;  // ? units: 1/m
			const double kDrag_y = 1.0;  // ? units: 1/m
			double f_x = f_sail * sin(rad(rudd.sail_deg)) - sqr(imu.vel_x_m_s) * kDrag_x * sign(imu.vel_x_m_s);
			double f_y = f_sail * cos(rad(rudd.sail_deg)) - sqr(imu.vel_y_m_s) * kDrag_y * sign(imu.vel_y_m_s);

			const double kMBoat = 535.0;   // mass of boat
			imu.acc_x_m_s2 = f_x / kMBoat;
			imu.acc_y_m_s2 = f_y / kMBoat;
      cap(&imu.acc_x_m_s2, 0.3);
      cap(&imu.acc_y_m_s2, 0.03);

			double deltat_s = 0.001 * (now - imu.timestamp_ms);
			imu.vel_x_m_s += deltat_s * imu.acc_x_m_s2;
			imu.vel_y_m_s += deltat_s * imu.acc_y_m_s2;
			
			// Cap the speed
			cap(&imu.vel_x_m_s, 3);     // 6 knots would be phantastic. 
			cap(&imu.vel_y_m_s, 0.15);  // The keel has a purpose. 

			double speed_m_s = sqrt(sqr(imu.vel_x_m_s) + sqr(imu.vel_y_m_s));
			double cog_rad = rad(imu.yaw_deg)-atan2(imu.vel_x_m_s, imu.vel_y_m_s);

			// Pretend we only turn because the rudder makes us describe a circle of radius D * tan(pi-rudder_deg)
			double rudder_deg = 0.5 * (rudd.rudder_l_deg + rudd.rudder_r_deg);
			if (fabs(rudder_deg) > 80.0) {
				imu.gyr_z_rad_s = 0;  // stall and tan() gets huge, not used anyway.
			} else {
				const double kRudderArm_m = 1.43;
				imu.gyr_z_rad_s = -speed_m_s * tan(rad(rudder_deg)) / kRudderArm_m;
			}
                        cap(&imu.gyr_z_rad_s, 0.8);  // 40 degree per second max turn rate
                        imu.yaw_deg += deltat_s * deg(imu.gyr_z_rad_s);

			double dist_m = deltat_s * speed_m_s;
			imu.lat_deg += dist_m * cos(cog_rad) / (1852.0 * 60.0);   // 1852 meters per (1/60) deg
			imu.lng_deg += dist_m * sin(cog_rad) / (1852.0 * 60.0 * cos(rad(imu.lat_deg)));
      CHECK(!isnan(imu.lat_deg));
      CHECK(!isnan(imu.lng_deg));
      
			imu.timestamp_ms = now;
		}
		
		if (now < last + 1000) continue;
		last = now;
                printf(OFMT_IMUPROTO(imu));
	}

	crash("Terminating.");
	
	return 0;
}
