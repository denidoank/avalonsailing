// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Once every 2 seconds, issue 2 sail motor current possition reads and in between
// one bmmh read, update 'skew' and send that to stdout.
//
//    plug -n skew $EBUS -- ./skew
// 

#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "proto/skew.h"
#include "timer.h"
#include "log.h"
#include "ebus.h"
#include "actuator.h"

// static const char* version = "$Id: $";
static const char* argv0;
static int debug = 0;

static void usage(void) {
	fprintf(stderr,	"usage: plug  /path/to/ebus -- %s [-T] \n", argv0);
	exit(2);
}

const double BMMH_BIAS_DEG = 3.25;  // if the boom is at 0, the BMMH reports 3.25 degrees

const int64_t REPORT_TIMEOUT_US = 8*1000*1000; // force new measurement if last report is 8s old.
const int64_t MOTOR_MAX_INTERVAL_US = 250*1000; // 250ms between motor currpos samples

int main(int argc, char* argv[]) {
	
	int ch;
	int dotimestamps = 0;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "dhT")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
		case 'T': ++dotimestamps; break;
		case 'h': 
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 0) usage();

	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");

	openlog(argv0, debug?LOG_PERROR:0, LOG_LOCAL2);

	printf("$name skewmon\n");
	printf("$subscribe 0x%x:0x%x[%d] = \n", motor_params[SAIL].serial_number, INDEX(REG_CURRPOS), SUBINDEX(REG_CURRPOS));
	printf("$subscribe 0x%x:0x%x[%d] = \n", motor_params[BMMH].serial_number, INDEX(REG_BMMHPOS), SUBINDEX(REG_BMMHPOS));

	struct SkewProto skew = INIT_SKEWPROTO;

	int64_t mc = 0;
	int32_t motor_currpos_qc[2] = { 0, 0 };
	int64_t motor_us[2] = { 0, 0 };
	int32_t bmmh_currpos_qc = 0;
	int64_t bmmh_us 	= 0;

	while(!feof(stdin)) {
		char line[1024];
		if (!fgets(line, sizeof(line), stdin))
			crash("reading stdin");
		
		uint32_t serial = 0;
		uint32_t reg    = 0;
		char op 	= 0;
		int32_t value 	= 0;
		uint64_t us 	= 0;

		if (ebus_parse_rsp(line, &op, &serial, &reg, &value, &us)) {
			if (us == 0) us = now_us();

			if (serial == motor_params[SAIL].serial_number && reg == REG_CURRPOS) {
				if(debug) slog(LOG_DEBUG, "Got sail 0x%x: %.2lf\n", value, qc_to_angle(&motor_params[SAIL], value));
				motor_currpos_qc[mc&1] = value;
				motor_us[mc&1] = us;
				++mc;
			}

			if (serial == motor_params[BMMH].serial_number && reg == REG_BMMHPOS) {
				if(value > (1<<29)) value -= (1<<30);
				value &= 4095;
				if(debug) slog(LOG_DEBUG, "Got bmmh 0x%x %d: %.2lf\n", value, value, qc_to_angle(&motor_params[BMMH], value));
				// bmmh position is 30 bit signed,0 .. 0x4000 0000 -> 0x2000 0000 => -0x2000 0000
				bmmh_currpos_qc = value;
				bmmh_us = us;
			}
		}

		if (us == 0) us = now_us();
		if (us - 1000*skew.timestamp_ms < REPORT_TIMEOUT_US/4) // limit at 1/4 of max period
			continue;

		double alpha = -1.0;
		int32_t motor_qc = 0;
		if (motor_us[0] < bmmh_us && bmmh_us < motor_us[1] && (motor_us[1] - motor_us[0] < MOTOR_MAX_INTERVAL_US) ) {
			alpha = bmmh_us - motor_us[0];
			alpha /= motor_us[1] - motor_us[0];
			motor_qc = (1.0-alpha)*motor_currpos_qc[0] + alpha*motor_currpos_qc[1];
			if(debug) slog(LOG_DEBUG, "< alpha %.3lf qc: 0x%x", alpha, motor_qc);
		} else if (motor_us[1] < bmmh_us && bmmh_us < motor_us[0] && (motor_us[0] - motor_us[1] < MOTOR_MAX_INTERVAL_US) ) {
			alpha = bmmh_us - motor_us[1];
			alpha /= motor_us[0] - motor_us[1];
			motor_qc = (1.0-alpha)*motor_currpos_qc[1] + alpha*motor_currpos_qc[0];
			if(debug) slog(LOG_DEBUG, "> alpha %.3lf qc: 0x%x", alpha, motor_qc);
		}

		if (alpha >= 0.0) {
			if(debug) slog(LOG_DEBUG, "average motor pos %.3lf", qc_to_angle(&motor_params[SAIL], motor_qc));
			skew.angle_deg = qc_to_angle(&motor_params[BMMH], bmmh_currpos_qc) - qc_to_angle(&motor_params[SAIL], motor_qc) - BMMH_BIAS_DEG;
			while(skew.angle_deg < -180.0) skew.angle_deg += 360.0;
			while(skew.angle_deg >  180.0) skew.angle_deg -= 360.0;
			skew.timestamp_ms = bmmh_us / 1000;
			printf(OFMT_SKEWPROTO(skew));
			fflush(stdout);
		}

		if (us - 1000*skew.timestamp_ms > REPORT_TIMEOUT_US) {
			// pretend we sent a bit ago to avoid flooding the bus
			skew.timestamp_ms = (us - REPORT_TIMEOUT_US/2)/1000;
			if (dotimestamps) {
				us = now_us();
				printf(EBUS_GET_T_OFMT(motor_params[SAIL].serial_number, REG_CURRPOS, us));
				printf(EBUS_GET_T_OFMT(motor_params[BMMH].serial_number, REG_BMMHPOS, us));
				printf(EBUS_GET_T_OFMT(motor_params[SAIL].serial_number, REG_CURRPOS, us));
			} else {
				printf(EBUS_GET_OFMT(motor_params[SAIL].serial_number, REG_CURRPOS));
				printf(EBUS_GET_OFMT(motor_params[BMMH].serial_number, REG_BMMHPOS));
				printf(EBUS_GET_OFMT(motor_params[SAIL].serial_number, REG_CURRPOS));
			}
			fflush(stdout);
		}
	}

	crash("main loop exit");

	return 0;
}
