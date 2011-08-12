#ifndef PROTO_HELMSMAN_H
#define PROTO_HELMSMAN_H

#include <math.h>
#include <stdint.h>

struct HelmsmanCtlProto {
	int64_t timestamp_ms;
	double alpha_star_deg;
	int brake;
	int dock;
};

#define INIT_HELMSMANCTLPROTO { 0, NAN, 0, 0 }

// For use in printf and friends.
#define OFMT_HELMSMANCTLPROTO(x, n) \
	"helm: timestamp_ms:%lld alpha_star_deg:%.3lf brake:%d dock:%d%n\n", \
	  (x).timestamp_ms, (x).alpha_star_deg, (x).brake, (x).dock, (n)

#define IFMT_HELMSMANCTLPROTO(x, n) \
	"helm: timestamp_ms:%lld alpha_star_deg:%lf brake:%d dock:%d%n\n", \
	  &(x)->timestamp_ms, &(x)->alpha_star_deg, &(x)->brake, &(x)->dock, (n)

#endif  // PROTO_HELMSMAN_H
