// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "eposclient.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static int64_t now_us() {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0) {
		fprintf(stderr, "no working clock");
		exit(1);
	}

	int64_t ms1 = tv.tv_sec;  ms1 *= 1000000;
	int64_t ms2 = tv.tv_usec;
	return ms1 + ms2;
}


enum { INVALID, PENDING, VALID };

typedef struct Register Register;

struct Register {
	Register *next;
	uint32_t reg;
	uint32_t value;
	int state;
	int64_t issued_us;
};

struct Device {
	Device *next;
	Bus *bus;
	uint32_t serial;
	Register* registers;
};

struct Bus {
	FILE* ctl;
	Device* devices;
	int timestamp;
};

Bus* bus_new(FILE* ctl) {
	Bus* bus = malloc(sizeof(*bus));
	bus->ctl = ctl;
	bus->devices = NULL;
	bus->timestamp = 0;
	return bus;
}

void bus_enable_timestamp(Bus* bus, int on) { bus->timestamp = on; }

int64_t bus_receive(Bus* bus, char* line) {

	uint32_t serial = 0;
	int index       = 0;
	int subindex    = 0;
	char op[3] = { 0, 0, 0 };
	int64_t value_l = 0;  // fumble signedness

	int n = sscanf(line, "%i:%i[%i] %2s %lli", &serial, &index, &subindex, op, &value_l);
	if (n != 5) return 0;

	// we only care about acks and nacks
	if(op[0] != '=' && op[0] != '#')
		return 0;

	uint32_t value = value_l;
	uint32_t idx = REGISTER(index, subindex);

	Device* dev = NULL;
	for (dev = bus->devices; dev; dev = dev->next)
		if (dev->serial == serial)
			break;

	if(!dev) return 0;

	Register* reg = NULL;
	for (reg = dev->registers; reg; reg = reg->next)
		if (reg->reg == idx)
			break;

	if (!reg) return 0;

	int prev = reg->state;

	if (op[0] == '=') {
		reg->value = value;
		reg->state = VALID;
	} else {
		reg->state = INVALID;
	}

	if (prev != PENDING)  // we got a response to someone elses request
		return 1;

	int64_t dt = now_us() - reg->issued_us;
	if (dt < 2) dt = 2;
	return dt;  // guard against clock jump
}

enum { TIMEOUT_US = 1*1000*1000 };  // 1 second

int bus_clocktick(Bus* bus) {
	Device* dev;
	Register* reg;
	int r = 0;
	int64_t t = now_us();
	for (dev = bus->devices; dev; dev = dev->next)
		for (reg = dev->registers; reg; reg = reg->next) {
			if (reg->state != PENDING) continue;
			// guard against clock jumps
			if ((t < reg->issued_us) || (t - reg->issued_us > TIMEOUT_US)) {
				reg->state = INVALID;
				++r;
			}
		}
	return r;
}

Device* bus_open_device(Bus* bus, uint32_t serial) {
	Device* dev;
	for (dev = bus->devices; dev; dev = dev->next)
		if (dev->serial == serial)
			return dev;
	dev = malloc(sizeof(*dev));
	dev->serial = serial;
	dev->registers = NULL;
	dev->bus = bus;
	dev->next = bus->devices;
	bus->devices = dev;
	return dev;
}

static Register* find_reg(Device* dev, uint32_t regidx) {
	Register* reg;
	for (reg = dev->registers; reg; reg = reg->next) {
		if (reg->reg == regidx)
			return reg;
	}

	reg = malloc(sizeof(*reg));
	reg->reg = regidx;
	reg->state = INVALID;
	reg->next = dev->registers;
	dev->registers = reg;
	return reg;
}

int device_get_register(Device* dev, uint32_t regidx, uint32_t* val) {
	Register* reg = find_reg(dev, regidx);
	int n;
	int64_t now;

	switch (reg->state) {
	case VALID:
		*val = reg->value;
		return 1;
	case INVALID:
		now = now_us();
		if (dev->bus->timestamp)
			n = fprintf(dev->bus->ctl, EBUS_GET_T_OFMT(dev->serial, regidx, now));
		else
			n = fprintf(dev->bus->ctl, EBUS_GET_OFMT(dev->serial, regidx));
		if (n > 0) {
			reg->state = PENDING;
			reg->issued_us = now;
		}
		// fallthrough
	case PENDING:
		return 0;
	}
	return 0;
}

int device_set_register(Device* dev, uint32_t regidx, uint32_t val) {
	Register* reg = find_reg(dev, regidx);
	int n;

	if (reg->state == PENDING && reg->value == val)
		return 0;

	if (reg->state == VALID && reg->value == val)
		return 1;

	const int64_t now = now_us();
	if (dev->bus->timestamp)
		fprintf(dev->bus->ctl, EBUS_SET_T_OFMT(dev->serial, regidx, val, now));
	else
		fprintf(dev->bus->ctl, EBUS_SET_OFMT(dev->serial, regidx, val));
	if (n > 0) {
		reg->value = val;
		reg->state = PENDING;
		reg->issued_us = now;
	} else {
		reg->state = INVALID;
	}
	return 0;
}

void device_invalidate_register(Device* dev, uint32_t regidx) {
	Register* reg;
	for (reg = dev->registers; reg; reg = reg->next)
		if (reg->reg == regidx)
			break;
	if (reg)
		reg->state = INVALID;
}

void device_invalidate_all(Device* dev) {
	Register* reg;
	for (reg = dev->registers; reg; reg = reg->next)
		reg->state = INVALID;
}
