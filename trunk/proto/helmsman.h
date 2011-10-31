#ifndef PROTO_HELMSMAN_H
#define PROTO_HELMSMAN_H

#include <math.h>
#include <stdint.h>

struct HelmsmanCtlProto {
	int64_t timestamp_ms;
	double alpha_star_deg;
};

#define INIT_HELMSMANCTLPROTO { 0, NAN }

// For use in printf and friends.
#define OFMT_HELMSMANCTLPROTO(x) \
	"helm: timestamp_ms:%lld alpha_star_deg:%.3lf\n", \
	  (x).timestamp_ms, (x).alpha_star_deg

#define IFMT_HELMSMANCTLPROTO(x, n) \
	"helm: timestamp_ms:%lld alpha_star_deg:%lf\n%n", \
	  &(x)->timestamp_ms, &(x)->alpha_star_deg, (n)

#endif  // PROTO_HELMSMAN_H
