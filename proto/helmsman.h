#ifndef PROTO_HELMSMAN_H
#define PROTO_HELMSMAN_H

#include <math.h>
#include <stdint.h>

struct HelmsmanCtlProto {
	int64_t timestamp_ms;
	double alpha_star_deg;
};

#define INIT_HELMSMANCTLPROTO { 0, NAN }

#define OFMT_HELMSMANCTLPROTO(x) \
	"helmctl: timestamp_ms:%lld alpha_star_deg:%.3lf\n",	\
		(x).timestamp_ms, (x).alpha_star_deg

#define IFMT_HELMSMANCTLPROTO(x, n) \
	"helmctl: timestamp_ms:%lld alpha_star_deg:%lf\n%n",	\
		&(x)->timestamp_ms, &(x)->alpha_star_deg, (n)


struct HelmsmanStsProto {
	int64_t timestamp_ms;
	double lat_deg, lon_deg;   // filtered position
	double wind_true_deg, wind_true_kn;   // filtered direction and speed of wind
};

#define INIT_HELMSMANSTSPROTO { 0, NAN, NAN, NAN, NAN }

#define OFMT_HELMSMANSTSPROTO(x)					\
	"helmsts: timestamp_ms:%lld lat_deg:%.7lf lon_deg:%.7lf wind_true_deg:%.2lf wind_true_kn:%.2lf\n", \
		(x).timestamp_ms, (x).lat_deg, (x).lon_deg, (x).wind_true_deg, (x).wind_true_kn

#define IFMT_HELMSMANSTSPROTO(x, n)					\
	"helmsts: timestamp_ms:%lld lat_deg:%lf lon_deg:%lf wind_true_deg:%lf wind_true_kn:%lf%n", \
		&(x)->timestamp_ms, &(x)->lat_deg, &(x)->lon_deg, &(x)->wind_true_deg, &(x)->wind_true_kn, (n)

#endif  // PROTO_HELMSMAN_H
