#ifndef PROTO_WIND_H
#define PROTO_WIND_H

#include <math.h>
#include <stdint.h>

struct WindProto {
	int64_t timestamp_ms;
	double angle_deg;
	int relative;
	double speed_m_s;
	int valid;
};

#define INIT_WINDPROTO {0, NAN, -1, NAN, 0}

// For use in printf and friends.
#define OFMT_WINDPROTO(x)						\
	"wind: timestamp_ms:%lld angle_deg:%.3lf speed_m_s:%.2lf valid:%d\n", \
		(x).timestamp_ms, (x).angle_deg, (x).speed_m_s, (x).valid

#define IFMT_WINDPROTO(x, n) \
	"wind: timestamp_ms:%lld angle_deg:%lf speed_m_s:%lf valid:%d%n", \
		&(x)->timestamp_ms, &(x)->angle_deg, &(x)->speed_m_s, &(x)->valid, (n)



// voltage and temperature report from the windsensor

struct WixdrProto {
	int64_t timestamp_ms;
	double temp_c, vheat_v, vsupply_v, vref_v;
};

#define INIT_WIXDRPROTO { 0, NAN, NAN, NAN, NAN }

#define OFMT_WIXDRPROTO(x)						\
	"wixdr: timestamp_ms:%lld temp_c:%.1lf vheat_v:%.2lf vsupply_v:%.2lf vref_v:%.2lf\n", \
		(x).timestamp_ms, (x).temp_c, (x).vheat_v, (x).vsupply_v, (x).vref_v



#endif // PROTO_WIND_H
