#ifndef PROTO_MODEM_H
#define PROTO_MODEM_H

#include <math.h>
#include <time.h>

struct ModemProto {
  time_t timestamp_s;
  int inbox_queue_messages;
  int outbox_queue_messages;
  int network_registration;
  int signal_quality;
  double coarse_position_lat;
  double coarse_position_lng;
  time_t position_timestamp_s;
};

#define INIT_MODEMPROTO {0, 0, 0, false, 0, NAN, NAN, 0}

// For use in printf and friends.
#define OFMT_MODEMPROTO(x)                                     \
        "modem: timestamp_ms:%ld inbox_queue_messages:%d "     \
        "outbox_queue_messages:%d network_registration:%d "    \
        "signal_quality:%d coarse_position_lat:%.3lf "         \
        "coarse_position_lng:%.3lf position_timestamp:%ld %n", \
        (x).timestamp_s, (x).inbox_queue_messages,             \
        (x).outbox_queue_messages, (x).network_registration,   \
        (x).signal_quality, (x).coarse_position_lat,           \
        (x).coarse_position_lng, (x).position_timestamp_s

#define IFMT_MODEMPROTO(x, n)                                    \
        "modem: timestamp_ms:%ld inbox_queue_messages:%d "       \
        "outbox_queue_messages:%d network_registration:%d "      \
        "signal_quality:%d coarse_position_lat:%lf "             \
        "coarse_position_lng:%lf position_timestamp:%ld %n",     \
        &(x)->timestamp_s, &(x)->inbox_queue_messages,           \
        &(x)->outbox_queue_messages, &(x)->network_registration, \
        &(x)->signal_quality, &(x)->coarse_position_lat,         \
        &(x)->coarse_position_lng, &(x)->position_timestamp_s, (n)

#endif // PROTO_MODEM_H
