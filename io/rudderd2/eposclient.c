// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "eposclient.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

enum { INVALID, PENDING, VALID };

typedef struct Register Register;

struct Register {
	Register *next;
	uint32_t reg;
	uint32_t value;
	int state;
	long issued_ms;
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
};

Bus* bus_new(FILE* ctl) {
	Bus* bus = malloc(sizeof(*bus));
        bus->ctl = ctl;
	bus->devices = NULL;
	return bus;
}

int bus_receive(Bus* bus, char* line) {

	uint32_t serial = 0;
	int index       = 0;
	int subindex    = 0;
	char op[3];
	int64_t value_l = 0;  // fumble signedness

	int n = sscanf(line, "%i:%i[%i] %2s %lli", &serial, &index, &subindex, op, &value_l);
	if (n != 5) return 0;

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
	
	if (op[0] == '=') {
		reg->value = value;
		reg->state = VALID;
	} else {
		reg->state = INVALID;
	}

	return 1;
}

// time in ms since first call.
static long now_ms() {
	static struct timeval tv_zero = { 0 , 0 };
	if (tv_zero.tv_sec == 0) {
		if (gettimeofday(&tv_zero, NULL) < 0) {
			fprintf(stderr, "No working clock:%s\n", strerror(errno));
			exit(1);
		}
		return 0;
	}

	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0) {
		fprintf(stderr, "No working clock:%s\n", strerror(errno));
		exit(1);
	}

	long ms = tv.tv_usec;
	ms -= tv_zero.tv_usec;
	ms /= 1000;
	return 1000 * (tv.tv_sec - tv_zero.tv_sec) + ms;
}

enum { TIMEOUT_MS = 1000 };

void bus_clocktick(Bus* bus) {
	Device* dev;
	Register* reg;
	long t = now_ms();
	for (dev = bus->devices; dev; dev = dev->next)
		for (reg = dev->registers; reg; reg = reg->next) {
			if (reg->state != PENDING) continue;
			// guard against clock jumps
			if ((t < reg->issued_ms) || (t - reg->issued_ms > TIMEOUT_MS))
				reg->state = INVALID;
		}
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
	for (reg = dev->registers; reg; reg = reg->next)
		if (reg->reg == regidx)
			return reg;

	reg = malloc(sizeof(*reg));
	reg->reg = regidx;
	reg->state = INVALID;
	reg->next = dev->registers;
	dev->registers = reg;
	return reg;
}

int device_get_register(Device* dev, uint32_t regidx, uint32_t* val) {
	Register* reg = find_reg(dev, regidx);

	switch (reg->state) {
	case VALID:
		*val = reg->value;
		return 1;
	case INVALID:
		if (fprintf(dev->bus->ctl, EBUS_GET_OFMT(dev->serial, regidx)) > 0) {
			reg->state = PENDING;
			reg->issued_ms = now_ms();
		}
		// fallthrough
	case PENDING:
		return 0;
	}
	return 0;
}

int device_set_register(Device* dev, uint32_t regidx, uint32_t val) {
	Register* reg = find_reg(dev, regidx);

	if (reg->state == PENDING && reg->value == val)
		return 0;

	if (reg->state == VALID && reg->value == val)
		return 1;

	if (fprintf(dev->bus->ctl, EBUS_SET_OFMT(dev->serial, regidx, val)) > 0) {
		reg->value = val;
		reg->state = PENDING;
		reg->issued_ms = now_ms();
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
