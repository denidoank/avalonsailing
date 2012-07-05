// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Meteo condition simulation: true wind including variable direction and gusts.

#ifndef FAKEIO_PROTO_METEO_H
#define FAKEIO_PROTO_METEO_H

#include <math.h>
#include <time.h>

struct MeteoProto {
  time_t timestamp_s;
  double true_wind_deg;
  double true_wind_speed_kt;
  int turbulence;  // 0: no turbulence, 5: light, 10: moderate, 50: storm.
};

#define INIT_METEOPROTO {0, NAN, NAN, 0}

// For use in printf and friends.
#define OFMT_METEOPROTO(x)                         \
  "meteo: timestamp_s:%ld true_wind_deg:%05.1lf "  \
  "true_wind_speed_kt:%.1lf turbulence:%d\n",      \
  (x).timestamp_s, (x).true_wind_deg,              \
  (x).true_wind_speed_kt, (x).turbulence

#define IFMT_METEOPROTO(x, n)                      \
  "meteo: timestamp_s:%ld true_wind_deg:%lf "      \
  "true_wind_speed_kt:%lf turbulence:%d\n%n",      \
  &(x)->timestamp_s, &(x)->true_wind_deg,          \
  &(x)->true_wind_speed_kt, &(x)->turbulence, (n)

#endif // FAKEIO_PROTO_METEO_H
