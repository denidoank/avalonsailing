#ifndef PROTO_GPS_H
#define PROTO_GPS_H

#include <math.h>
#include <stdint.h>

struct GPSProto {
	int64_t timestamp_ms;
	int64_t gps_timestamp_ms;
	double lat_deg;
	double lng_deg;
	double speed_m_s;
	double cog_deg;
};

#define INIT_GPSPROTO {0,0,NAN,NAN,NAN,NAN}

// For use in printf and friends.
#define OFMT_GPSPROTO(x)						\
	"gps: timestamp_ms:%lld gps_timestamp_ms:%lld lat_deg:%.7lf lng_deg:%.7lf speed_m_s:%.3lf cog_deg:%.2lf\n", \
		(x).timestamp_ms, (x).gps_timestamp_ms, (x).lat_deg, (x).lng_deg, (x).speed_m_s, (x).cog_deg

#define IFMT_GPSPROTO(x, n)				 \
	"gps: timestamp_ms:%lld gps_timestamp_ms:%lld lat_deg:%lf lng_deg:%lf speed_m_s:%lf cog_deg:%lf%n", \
		&(x)->timestamp_ms, &(x)->gps_timestamp_ms, &(x)->lat_deg, &(x)->lng_deg, &(x)->speed_m_s, &(x)->cog_deg, (n)

#endif  // PROTO_GPS_H
