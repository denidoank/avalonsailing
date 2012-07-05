#ifndef PROTO_FUELCELL_H
#define PROTO_FUELCELL_H

#include <math.h>
#include <stdint.h>

struct FuelcellProto {
	int64_t timestamp_ms;
	double voltage_V;
	double charge_current_A;
	double energy_Wh;
	double runtime_h;
};

#define INIT_FUELCELLPROTO {0, NAN, NAN, NAN, NAN}

#define OFMT_FUELCELLPROTO(x)						\
	"fuelcell: timestamp_ms:%lld voltage_V:%.3lf charge_current_A:%.3lf energy_Wh:%.3lf runtime_h:%.3lf\n",	\
		(x).timestamp_ms, (x).voltage_V, (x).charge_current_A, (x).energy_Wh, (x).runtime_h

#define IFMT_FUELCELLPROTO(x, n)					\
	"fuelcell: timestamp_ms:%lld voltage_V:%lf charge_current_A:%lf energy_Wh:%lf runtime_h:%lf%n",	\
		&(x)->timestamp_ms, &(x)->voltage_V, &(x)->charge_current_A, &(x)->energy_Wh, &(x)->runtime_h, (n)
	
#endif // PROTO_FUELCELL_H
