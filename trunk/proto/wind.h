#ifndef PROTO_WIND_H
#define PROTO_WIND_H

#include <math.h>

struct WindProto {
	uint64_t timestamp_ms;
	double angle_deg;
	int relative;
	double speed_m_s;
	int valid;
};

#define INIT_WINDPROTO \
  {0, NAN, -1, NAN, 0}

// For use in printf and friends.
#define OFMT_WINDPROTO(x, n) \
	"wind: timestamp_ms:%lld angle_deg:%.3lf speed_m_s:%.2lf valid:%d%n", \
	(x).timestamp_ms, (x).angle_deg, (x).speed_m_s, (x).valid, (n)

#define IFMT_WINDPROTO(x, n) \
	"wind: timestamp_ms:%lld angle_deg:%lf speed_m_s:%lf valid:%d%n", \
	&(x)->.timestamp_ms, &(x)->angle_deg, &(x)->speed_m_s, &(x)->valid, (n)

#endif // PROTO_WIND_H
