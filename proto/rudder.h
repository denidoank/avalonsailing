#ifndef PROTO_RUDDER_H
#define PROTO_RUDDER_H

#include <math.h>

struct RudderProto {
	int64_t timestamp_ms;
	double rudder_l_deg;
	double rudder_r_deg;
	double sail_deg;
};

#define INIT_RUDDERPROTO { 0, NAN, NAN, NAN }

#define OFMT_RUDDERPROTO_STS(x,n) \
	"ruddersts: timestamp_ms:%lld rudder_l_deg:%.1f rudder_r_deg:%.1f sail_deg:%.1f " \
	, (x).timestamp_ms, (x).rudder_l_deg, (x).rudder_r_deg, (x).sail_deg

#define OFMT_RUDDERPROTO_CTL(x,n) \
	"ruddersts: timestamp_ms:%lld rudder_l_deg:%.1lf rudder_r_deg:%.1lf sail_deg:%.1lf " \
	, (x).timestamp_ms, (x).rudder_l_deg, (x).rudder_r_deg, (x).sail_deg

#define IFMT_RUDDERPROTO_STS(x,n) \
	"rudderctl: timestamp_ms:%lld rudder_l_deg:%lf rudder_r_deg:%lf sail_deg:%lf " \
	, &(x)->timestamp_ms, &(x)->rudder_l_deg, &(x)->rudder_r_deg, &(x)->sail_deg

// CTL input is parsed by hand

#endif // PROTO_RUDDER_H
