#ifndef PROTO_RUDDER_H
#define PROTO_RUDDER_H

#include <math.h>
#include <stdint.h>

struct RudderProto {
	int64_t timestamp_ms;
	double rudder_l_deg;
	double rudder_r_deg;
	double sail_deg;
};

#define INIT_RUDDERPROTO { 0, NAN, NAN, NAN }

#define OFMT_RUDDERPROTO_STS(x)						\
	"ruddersts: timestamp_ms:%lld rudder_l_deg:%.1lf rudder_r_deg:%.1lf sail_deg:%.1lf\n", \
		(x).timestamp_ms, (x).rudder_l_deg, (x).rudder_r_deg, (x).sail_deg

#define OFMT_RUDDERPROTO_CTL(x)						\
	"rudderctl: timestamp_ms:%lld rudder_l_deg:%.3lf rudder_r_deg:%.3lf sail_deg:%.1lf\n", \
		(x).timestamp_ms, (x).rudder_l_deg, (x).rudder_r_deg, (x).sail_deg

#define IFMT_RUDDERPROTO_STS(x, n)					\
	"ruddersts: timestamp_ms:%lld rudder_l_deg:%lf rudder_r_deg:%lf sail_deg:%lf%n", \
		&(x)->timestamp_ms, &(x)->rudder_l_deg, &(x)->rudder_r_deg, &(x)->sail_deg, (n)
#define IFMT_RUDDERPROTO_STS_ITEMS 4


#define IFMT_RUDDERPROTO_CTL(x, n)					\
	"rudderctl: timestamp_ms:%lld rudder_l_deg:%lf rudder_r_deg:%lf sail_deg:%lf%n", \
		&(x)->timestamp_ms, &(x)->rudder_l_deg, &(x)->rudder_r_deg, &(x)->sail_deg, (n)
#define IFMT_RUDDERPROTO_CTL_ITEMS 4


#define OFMT_STATUS_LEFT(x)						\
	"status_left: timestamp_ms:%lld angle_deg:%.1lf\n", (x).timestamp_ms, (x).rudder_l_deg

#define OFMT_STATUS_RIGHT(x)						\
	"status_right: timestamp_ms:%lld angle_deg:%.1lf\n", (x).timestamp_ms, (x).rudder_r_deg

#define OFMT_STATUS_SAIL(x)						\
	"status_sail: timestamp_ms:%lld angle_deg:%.1lf\n", (x).timestamp_ms, (x).sail_deg

#define IFMT_STATUS_LEFT(x, n) \
	"status_left: timestamp_ms:%lld angle_deg:%lf%n", &(x)->timestamp_ms, &(x)->rudder_l_deg, (n)

#define IFMT_STATUS_RIGHT(x, n)						\
	"status_right: timestamp_ms:%lld angle_deg:%lf%n", &(x)->timestamp_ms, &(x)->rudder_r_deg, (n)

#define IFMT_STATUS_SAIL(x, n)						\
	"status_sail: timestamp_ms:%lld angle_deg:%lf%n", &(x)->timestamp_ms, &(x)->sail_deg, (n)

#endif // PROTO_RUDDER_H
