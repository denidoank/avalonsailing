#include "ebus.h"

#include <assert.h>
#include <stdio.h>

int main(int argc, char* argv[]) {

	char buf[100];

	uint32_t serial = 0;
	uint32_t regidx = 0;
	char op 	= 0;
	int32_t value 	= 0;
	uint64_t us 	= 0; 

	assert(snprintf(buf, sizeof buf, EBUS_GET_OFMT(0x1234, REGISTER(0x1001, 2))) < sizeof buf);

	assert(ebus_parse(buf, &op, &serial, &regidx, &value, &us));
	assert(op == '?');
	assert(serial == 0x1234);
	assert(regidx == 0x100102);
	assert(value == 0);
	assert(us == 0);

	serial = 0;
	regidx = 0;
	op 	= 0;
	value 	= 0;
	us 	= 0; 

	assert(snprintf(buf, sizeof buf, EBUS_SET_OFMT(0x1234, REGISTER(0x1001, 2), 0x4321)) < sizeof buf);

	assert(ebus_parse(buf, &op, &serial, &regidx, &value, &us));
	assert(op == ':');
	assert(serial == 0x1234);
	assert(regidx == 0x100102);
	assert(value == 0x4321);
	assert(us == 0);

	serial = 0;
	regidx = 0;
	op 	= 0;
	value 	= 0;
	us 	= 0; 

	assert(snprintf(buf, sizeof buf, EBUS_ACK_OFMT(0x1234, REGISTER(0x1001, 2), 0x4321)) < sizeof buf);

	assert(ebus_parse(buf, &op, &serial, &regidx, &value, &us));
	assert(op == '=');
	assert(serial == 0x1234);
	assert(regidx == 0x100102);
	assert(value == 0x4321);
	assert(us == 0);

	serial = 0;
	regidx = 0;
	op 	= 0;
	value 	= 0;
	us 	= 0x12345678; 

	assert(snprintf(buf, sizeof buf, EBUS_GET_T_OFMT(0x1234, REGISTER(0x1001, 2), us)) < sizeof buf);
	us = 0;

	assert(ebus_parse(buf, &op, &serial, &regidx, &value, &us));
	assert(op == '?');
	assert(serial == 0x1234);
	assert(regidx == 0x100102);
	assert(value == 0);
	assert(us == 0x12345678);

	serial = 0;
	regidx = 0;
	op 	= 0;
	value 	= 0;
	us 	= 0x12345678; 

	assert(snprintf(buf, sizeof buf, EBUS_SET_T_OFMT(0x1234, REGISTER(0x1001, 2), 0x4321, us)) < sizeof buf);
	us = 0;
	assert(ebus_parse(buf, &op, &serial, &regidx, &value, &us));
	assert(op == ':');
	assert(serial == 0x1234);
	assert(regidx == 0x100102);
	assert(value == 0x4321);
	assert(us == 0x12345678);

	serial = 0;
	regidx = 0;
	op 	= 0;
	value 	= 0;
	us 	= 0x12345678; 

	assert(snprintf(buf, sizeof buf, EBUS_ACK_T_OFMT(0x1234, REGISTER(0x1001, 2), 0x4321, us)) < sizeof buf);
	us = 0;

	assert(ebus_parse(buf, &op, &serial, &regidx, &value, &us));
	assert(op == '=');
	assert(serial == 0x1234);
	assert(regidx == 0x100102);
	assert(value == 0x4321);
	assert(us == 0x12345678);

	serial = 0;
	regidx = 0;
	op 	= 0;
	value 	= 0;
	us 	= 0x12345678; 

	assert(snprintf(buf, sizeof buf, EBUS_ERR_T_OFMT(0x1234, REGISTER(0x1001, 2), 0x4321, us)) < sizeof buf);
	us = 0;

	assert(ebus_parse(buf, &op, &serial, &regidx, &value, &us));
	assert(op == '#');
	assert(serial == 0x1234);
	assert(regidx == 0x100102);
	assert(value == 0x4321);
	assert(us == 0x12345678);


	puts("OK");
	return 0;
}
