#ifndef PROTO_MODEM_H
#define PROTO_MODEM_H

#include <math.h>
#include <stdint.h>

// irridium provides coarse grained lat/long
struct ModemProto {
	int64_t timestamp_ms;
	double lat_deg, lng_deg;
};

#define INIT_MODEMPROTO {0, NAN, NAN}

#define OFMT_MODEMPROTO(x)						\
	"modem: timestamp_ms:%lld lat_deg:%.7lf lng_deg:%.7lf\n",	\
		(x).timestamp_ms, (x).lat_deg, (x).lng_deg

#define IFMT_MODEMPROTO(x, n)						\
        "modem: timestamp_ms:%lld lat_deg:%lf lng_deg:%lf%n",		\
		&(x)->timestamp_ms, &(x)->lat_deg, &(x)->lng_deg, (n)

#endif // PROTO_MODEM_H
