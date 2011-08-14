// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Grundmann, August 2011
// Skipper input (position and true wind).

#ifndef PROTO_SKIPPER_INPUT_H
#define PROTO_SKIPPER_INPUT_H

#include <math.h>
#include <time.h>
#include <stdint.h>

struct SkipperInputProto {
  int64_t timestamp_ms;
  // GPS data in degrees
  // Both values are NAN initially.
  double longitude_deg;
  double latitude_deg;

  // true wind, global frame
  // Both values are NAN initially.
  double angle_true_deg;
  double mag_true_kn;
};

#define INIT_SKIPPER_INPUTPROTO {0, NAN, NAN, NAN, NAN}

// For use in printf and friends.
#define OFMT_SKIPPER_INPUTPROTO(x, n)  \
  "skipper_input: timestamp_ms:%lld longitude_deg:%lf latitude_deg:%lf angle_true_deg:%6.2lf mag_true_kn:%6.2lf%n\n", \
   (x).timestamp_ms, (x).longitude_deg, (x).latitude_deg, (x).angle_true_deg, (x).mag_true_kn, (n)


// For use in scanf.
#define IFMT_SKIPPER_INPUTPROTO(x, n)                             \
  "skipper_input: timestamp_ms:%lld longitude_deg:%lf latitude_deg:%lf angle_true_deg:%lf mag_true_kn:%lf%n",	   \
   &(x).timestamp_ms, &(x).longitude_deg, &(x).latitude_deg, &(x).angle_true_deg, &(x).mag_true_kn, (n)

#endif // PROTO_SKIPPER_INPUT_H
