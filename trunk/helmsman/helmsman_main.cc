// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Helmsman daemon for conrolling boat heading and sail
// ./helmsman --logtostderr --no-syslog --task-name=helmsman --timeout=10 --debug \
//        --imu=testdata/imu.nme --wind=testdata/wind.nme \
//        --drives=testdata/drive_in.nme \
//        --command=testdata/commands.txt --skipper_file=testdata/skipper.txt
// ./helmsman --logtostderr --no-syslog --task-name=helmsman --timeout=10 \
//       --debug  --imu=testdata/imu.nme --wind=testdata/wind.nme  --drives=testdata/drive_in.nme  --command=testdata/commands.txt --skipper=testdata/skipper.txt


#include "io/ipc/producer_consumer.h"
#include "lib/fm/fm.h"
#include "lib/fm/log.h"
//#include "modem/modem.h"

#include "helmsman/drive_data.h"
#include "helmsman/fill.h"
#include "helmsman/imu.h"

#include "helmsman/sampling_period.h"
#include "helmsman/ship_control.h"
#include "helmsman/wind.h"
#include "lib/util/stopwatch.h"

#include <getopt.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

using namespace std;

const size_t kFlagSize = 256;

static char imu_file[kFlagSize] = "";
static char wind_file[kFlagSize] = "";
static char drives_file[kFlagSize] = "";
static char skipper_file[kFlagSize] = "";
static char command_file[kFlagSize] = "";


static void Init(int argc, char** argv) {
  static const struct option long_options[] = {
    { "imu", 1, NULL, 0},
    { "wind", 1, NULL, 0},
    { "drives", 1, NULL, 0},
    { "command", 1, NULL, 0},
    { "skipper", 1, NULL, 0},
    { NULL, 0, NULL, 0 }
  };
  
  optind = 1;
  opterr = 0;
  int opt_index;
  int opt;
  while ((opt = getopt_long(argc, argv, "", long_options, &opt_index)) != -1) {
    if (opt != 0)  // Invalid argument, probably used by other module.
      continue;
    switch(opt_index) {
      case 0:  // --imu=...
        strncpy(imu_file, optarg, kFlagSize);
        break;
      case 1:  // --wind=...
        strncpy(wind_file, optarg, kFlagSize);
        break;
      case 2:  // --drives=...
        strncpy(drives_file, optarg, kFlagSize);
        break;
      case 3:  // --commands=...
        strncpy(command_file, optarg, kFlagSize);
        break;
      case 4:  // --skipper=...
        strncpy(skipper_file, optarg, kFlagSize);
        break;
      default:
        FM_LOG_INFO("usage: %s -i imu_dev] [-w wind_dev]\n"
                    "\t -d drives\n"
                    "\t -c command_dev -s skipper_dev\n",
                    argv[0]);
        exit(0);
    }
  }
}

long long NowMicros() {
  return StopWatch::GetTimestampMicros();
}

long long WaitUntil(long long next, long long period) {
  long long now = NowMicros();
  if (next > now) {
    sleep((next - now) / 1E6);
  } else {
    FM_LOG_INFO("Helmsman: to late %lld micros", now - next);
  }
  return next + period;
}


int main(int argc, char** argv) {
  FM::Init(argc, argv);
  Init(argc, argv);

  FM_LOG_INFO("Helmsman: Using IO files: %s and wind sensor: %s "
              "drives_file %s command_file %s skipper_file%s\n",
              imu_file, wind_file, drives_file, command_file, skipper_file);
  if (strlen(imu_file) == 0) {
    FM_LOG_FATAL("No imu_file specified.");
  }
  if (strlen(wind_file) == 0) {
    FM_LOG_FATAL("No wind_file specified.");
  }
  if (strlen(drives_file) == 0) {
    FM_LOG_FATAL("No drives_file specified.");
  }
  if (strlen(command_file) == 0) {
    FM_LOG_FATAL("No command_file specified.");
  }
  if (strlen(skipper_file) == 0) {
    FM_LOG_FATAL("No skipper_file specified.");
  }

  ShipControl m;
  Consumer imu_consumer(imu_file);
  Consumer wind_consumer(wind_file);
  Consumer command_consumer(command_file);
  Consumer skipper_consumer(skipper_file);
  ProducerConsumer drive(drives_file);
  //Producer skipper_producer(true_wind_file);

  string imu_raw, wind_raw, drives_raw, command_raw, skipper_raw;
  Imu imu;
  WindRad wind;

  if (imu_consumer.Consume(&imu_raw)) {
    FM_LOG_INFO("Found imu data: %s \n", imu_raw.c_str());
    FillImu(imu_raw, &imu);
  }
  if (wind_consumer.Consume(&wind_raw)) {
    FM_LOG_INFO("Found wind data: %s \n", wind_raw.c_str());
    FillWind(wind_raw, &wind);
  }
  if (command_consumer.Consume(&command_raw)) {
    FM_LOG_INFO("Found command data: %s \n", command_raw.c_str());
  }
  if (skipper_consumer.Consume(&skipper_raw)) {
    FM_LOG_INFO("Found skipper data: %s \n", skipper_raw.c_str());
  }

  DriveReferenceValuesRad drive_reference;

  int rounds = 0;
  long long next_micros = NowMicros() + kSamplingPeriod * 1E6;
  // wait until the next tick passed.
  next_micros = WaitUntil(next_micros, kSamplingPeriod * 1E6);
  while (true) {
    if (imu_consumer.Consume(&imu_raw)) {
      FM_LOG_INFO("Found imu data: %s \n", imu_raw.c_str());
      FillImu(imu_raw, &imu);
    }

    string line;
    drive.Consume(&line);
    DriveActualValuesRad drive_actual(line);
    FM_LOG_INFO("d actual: l %6.4f r %6.4f s %6.4f\n",
        drive_actual.homed_rudder_left  ? Rad2Deg(drive_actual.gamma_rudder_left_rad)  : -999,
        drive_actual.homed_rudder_right ? Rad2Deg(drive_actual.gamma_rudder_right_rad) : -999,
        drive_actual.homed_sail         ? Rad2Deg(drive_actual.gamma_sail_rad)         : -999);

    drive_reference.Reset();

    // A few steps in the reference values as test signal to measure max rotation speeds    
    int phase = (rounds % 600) / 60;  // 1 cycle / minute, each phase has 10s
    drive_reference.gamma_rudder_left_rad  = phase == 0 ? Deg2Rad(15) : -Deg2Rad(15);
    drive_reference.gamma_rudder_right_rad = phase == 2 ? Deg2Rad(15) : -Deg2Rad(15);
    drive_reference.gamma_sail_rad =         phase == 4 ? Deg2Rad(20) : -Deg2Rad(20);

    {
      DriveReferenceValues out(drive_reference);
      drive.Produce(out.ToString());
      FM_LOG_INFO("d ref: t %6.5f l %6.4f r %6.4f s %6.4f\n",
          NowMicros() ? 1.0E6,
          drive_reference.gamma_rudder_star_left,
          drive_reference.gamma_rudder_star_right,
          drive_reference.gamma_sail_star);
    }

    FM::Keepalive();

    next_micros = WaitUntil(next_micros, kSamplingPeriod * 1E6);
    ++rounds;
  }
}
