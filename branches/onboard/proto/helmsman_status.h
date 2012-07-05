// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Grundmann, August 2011
// Skipper input (position and true wind).

#ifndef PROTO_HELMSMAN_STATUS_H
#define PROTO_HELMSMAN_STATUS_H

#include <math.h>
#include <time.h>
#include <stdint.h>

struct HelmsmanStatusProto {
  int64_t timestamp_ms;
  int tacks;
  int jibes;
  int inits;  // How many times the initial controlling state was entered
  // true wind, global frame
  // Both values are NAN initially.
  double direction_true_deg;
  double mag_true_m_s;
};

#define INIT_HELMSMAN_STATUSPROTO {0, 0, 0, 0, NAN, NAN}

// For use in printf and friends.
#define OFMT_HELMSMAN_STATUSPROTO(x)                                          \
   "helmsman_st: timestamp_ms:%lld tacks:%d jibes:%d inits:%d "               \
   "direction_true_deg:%.2lf mag_true_m_s:%.2lf\n",                           \
   (x).timestamp_ms, (x).tacks, (x).jibes, (x).inits, (x).direction_true_deg, \
   (x).mag_true_m_s


// For use in scanf.
#define IFMT_HELMSMAN_STATUSPROTO(x, n)                                       \
  "helmsman_st: timestamp_ms:%lld tacks:%d jibes:%d inits:%d "                \
  "direction_true_deg:%lf mag_true_m_s:%lf\n%n",                              \
   &(x)->timestamp_ms, &(x)->tacks, &(x)->jibes, &(x)->inits,                 \
   &(x)->direction_true_deg, &(x)->mag_true_m_s, (n)

#endif // PROTO_HELMSMAN_STATUS_H
