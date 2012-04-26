// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Digital compass status message.

#ifndef PROTO_COMPASS_H
#define PROTO_COMPASS_H

#include <math.h>
#include <stdint.h>

struct CompassProto {
  int64_t timestamp_ms;
  double yaw_deg;
  double pitch_deg;
  double roll_deg;
  double temp_c;
};

#define INIT_COMPASSPROTO \
  {0, NAN, NAN, NAN, NAN}

// For use in printf and friends.
#define OFMT_COMPASSPROTO(x)                                                           \
  "compass: timestamp_ms:%lld roll_deg:%.3lf pitch_deg:%.3lf yaw_deg:%.3lf temp_c:%.3lf\n",     \
  (x).timestamp_ms, (x).roll_deg, (x).pitch_deg, (x).yaw_deg, (x).temp_c

#define IFMT_COMPASSPROTO(x, n)                                                        \
  "compass: timestamp_ms:%lld roll_deg:%lf pitch_deg:%lf yaw_deg:%lf temp_c:%lf\n%n",           \
  &(x)->timestamp_ms, &(x)->roll_deg, &(x)->pitch_deg, &(x)->yaw_deg, &(x)->temp_c, (n)

#endif  // PROTO_COMPASS_H
