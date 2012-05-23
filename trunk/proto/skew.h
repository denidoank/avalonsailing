#ifndef PROTO_SKEW_H
#define PROTO_SKEW_H

#include <math.h>
#include <stdint.h>

struct SkewProto {
	int64_t timestamp_ms;
	double angle_deg;      // bmmh reported (actual) angle - motor reported angle
};

#define INIT_SKEWPROTO {0, NAN}

// For use in printf and friends.
#define OFMT_SKEWPROTO(x)    "skew: timestamp_ms:%lld angle_deg:%.3lf\n", (x).timestamp_ms, (x).angle_deg

#define IFMT_SKEWPROTO(x, n) "skew: timestamp_ms:%lld angle_deg:%lf\n%n", &(x)->timestamp_ms, &(x)->angle_deg, (n)

#endif // PROTO_WIND_H
