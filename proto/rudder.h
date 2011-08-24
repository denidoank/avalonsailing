#ifndef PROTO_RUDDER_H
#define PROTO_RUDDER_H

#include <math.h>
#include <stdint.h>

struct RudderProto {
	int64_t timestamp_ms;
	double rudder_l_deg;
	double rudder_r_deg;
	double sail_deg;
	int storm_flag;
};

#define INIT_RUDDERPROTO { 0, NAN, NAN, NAN, 0 }

#define OFMT_RUDDERPROTO_STS(x,n) \
	"ruddersts: timestamp_ms:%lld rudder_l_deg:%.1f rudder_r_deg:%.1f sail_deg:%.1f%n\n" \
	, (x).timestamp_ms, (x).rudder_l_deg, (x).rudder_r_deg, (x).sail_deg, (n)

#define OFMT_RUDDERPROTO_CTL(x,n) \
	"rudderctl: timestamp_ms:%lld rudder_l_deg:%.3lf rudder_r_deg:%.3lf sail_deg:%.1lf storm_flag:%d%n\n" \
	, (x).timestamp_ms, (x).rudder_l_deg, (x).rudder_r_deg, (x).sail_deg, (x).storm_flag, (n)

#define IFMT_RUDDERPROTO_STS(x,n) \
	"ruddersts: timestamp_ms:%lld rudder_l_deg:%lf rudder_r_deg:%lf sail_deg:%lf %n" \
	, &(x)->timestamp_ms, &(x)->rudder_l_deg, &(x)->rudder_r_deg, &(x)->sail_deg, (n)

#define IFMT_RUDDERPROTO_CTL(x,n) \
	"rudderctl: timestamp_ms:%lld rudder_l_deg:%lf rudder_r_deg:%lf sail_deg:%lf storm_flag:%d %n" \
	, &(x)->timestamp_ms, &(x)->rudder_l_deg, &(x)->rudder_r_deg, &(x)->sail_deg, &(x)->storm_flag, (n)


#endif // PROTO_RUDDER_H
