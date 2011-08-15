// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Remote control protocol.

#ifndef PROTO_REMOTE_H
#define PROTO_REMOTE_H

#include <math.h>
#include <time.h>

struct RemoteProto {
  time_t timestamp_s;
  // Command type:
  // bit 0-3:
  //  0 = not valid
  //  1 = normal mode (autonomous)
  //  2 = docking mode (sails, ruders straight)
  //  3 = brake mode 
  //  4 = override bearing mode
  //  5 = power shutdown
  // bit 4: force status report/ack.
  // bit 5-7: unused
  int command;
  double alpha_star_deg;  // valid if command = 4 only.
};

static const int kNormalControlMode = 1;
static const int kDockingControlMode = 2;
static const int kBrakeControlMode = 3;
static const int kOverrideSkipperMode = 4;
static const int kPowerShutdownMode = 5;

#define INIT_REMOTEPROTO {0, 0, NAN}

// For use in printf and friends.
#define OFMT_REMOTEPROTO(x, n)                                \
  "remote: timestamp_s:%ld command:%d alpha_star:%06.2lf%n",  \
    (x).timestamp_s, (x).command, (x).alpha_star_deg, (n)

#define IFMT_REMOTEPROTO(x, n)                                 \
  "remote: timestamp_s:%ld command:%d alpha_star:%lf%n",       \
   &(x)->timestamp_s, &(x)->command, &(x)->alpha_star_deg, (n)

#endif // PROTO_REMOTE_H
