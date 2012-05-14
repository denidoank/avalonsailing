#ifndef PROTO_DRIVE_STATUS_H
#define PROTO_DRIVE_STATUS_H

#include <math.h>
#include <stdint.h>

struct StatusLeftProto {
  int64_t timestamp_ms;
  double rudder_l_deg;
};

struct StatusRightProto {
  int64_t timestamp_ms;
  double rudder_r_deg;
};

struct StatusSailProto {
  int64_t timestamp_ms;
  double sail_deg;
};

#define INIT_STATUSPROTO { 0, NAN }

#define OFMT_STATUS_LEFT_PROTO(x) \
  "status_left: timestamp_ms:%lld rudder_l_deg:%.1lf\n" \
  , (x).timestamp_ms, (x).rudder_l_deg

#define IFMT_STATUS_LEFT_PROTO(x, n) \
  "status_left: timestamp_ms:%lld rudder_l_deg:%lf\n%n" \
  , &(x)->timestamp_ms, &(x)->rudder_l_deg, (n)


#define OFMT_STATUS_RIGHT_PROTO(x) \
  "status_right: timestamp_ms:%lld rudder_l_deg:%.1lf\n" \
  , (x).timestamp_ms, (x).rudder_r_deg,

#define IFMT_STATUS_RIGHT_PROTO(x, n) \
  "status_right: timestamp_ms:%lld rudder_r_deg:%lf\n%n" \
  , &(x)->timestamp_ms, &(x)->rudder_r_deg, (n)


#define OFMT_STATUS_SAIL_PROTO(x) \
  "status_sail: timestamp_ms:%lld rudder_l_deg:%.1lf\n" \
  , (x).timestamp_ms, (x).sail_deg

#define IFMT_STATUS_SAIL_PROTO(x, n) \
  "status_sail: timestamp_ms:%lld sail_deg:%lf\n%n" \
  , &(x)->timestamp_ms, &(x)->sail_deg, (n)

#endif // PROTO_DRIVE_STATUS_H
