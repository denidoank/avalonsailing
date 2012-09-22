#ifndef PROTO_HELMSMAN_H
#define PROTO_HELMSMAN_H

#include <math.h>
#include <stdint.h>

struct HelmsmanCtlProto {
	int64_t timestamp_ms;
	double alpha_star_deg;
	int tc_index;
	double tc_lat_deg;
	double tc_lon_deg;
};

#define INIT_HELMSMANCTLPROTO { 0, NAN, 0, NAN, NAN }

// For use in printf and friends.
#define OFMT_HELMSMANCTLPROTO(x) \
	"helm: timestamp_ms:%lld alpha_star_deg:%.3lf tc_index:%d tc_lat:%.7lf tc_lon:%.7lf\n", \
	  (x).timestamp_ms, (x).alpha_star_deg, (x).tc_index, (x).tc_lat_deg, (x).tc_lon_deg

#define IFMT_HELMSMANCTLPROTO(x, n) \
	"helm: timestamp_ms:%lld alpha_star_deg:%lf tc_index:%d tc_lat:%lf tc_lon:%lf\n%n", \
	  &(x)->timestamp_ms, &(x)->alpha_star_deg, &(x)->tc_index, &(x)->tc_lat_deg, &(x)->tc_lon_deg,(n)

#endif  // PROTO_HELMSMAN_H
