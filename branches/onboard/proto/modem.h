#ifndef PROTO_MODEM_H
#define PROTO_MODEM_H

#include <math.h>
#include <stdint.h>

struct ModemProto {
	int64_t timestamp_ms;
	int inbox_queue_messages;
	int outbox_queue_messages;
	int network_registration;
	int signal_quality;
	// irridium provides coarse grained lat/long
	double lat_deg, lng_deg;
	int64_t position_timestamp_ms;
};

#define INIT_MODEMPROTO {0, 0, 0, 0, 0, NAN, NAN, 0}

// For use in printf and friends.
#define OFMT_MODEMPROTO(x)						\
        "modem: timestamp_ms:%lld inbox_queue_messages:%d outbox_queue_messages:%d network_registration:%d signal_quality:%d " \
	"lat_deg:%.7lf lng_deg:%.7lf position_timestamp_ms:%lld\n",	\
		(x).timestamp_ms, (x).inbox_queue_messages, (x).outbox_queue_messages, (x).network_registration, (x).signal_quality, \
		(x).lat_deg, (x).lng_deg, (x).position_timestamp_ms

#define IFMT_MODEMPROTO(x, n)						\
        "modem: timestamp_ms:%lld inbox_queue_messages:%d outbox_queue_messages:%d network_registration:%d signal_quality:%d " \
	"lat_deg:%lf lng_deg:%lf position_timestamp_ms:%lld%n",		\
		&(x)->timestamp_ms, &(x)->inbox_queue_messages, &(x)->outbox_queue_messages, \
		&(x)->network_registration, &(x)->signal_quality,	\
		&(x)->lat_deg, &(x)->lng_deg, &(x)->position_timestamp_ms, (n)

#endif // PROTO_MODEM_H
