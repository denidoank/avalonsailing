// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "eposclient.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ebus.h"
#include "../timer.h"

enum { INVALID, PENDING, VALID };

typedef struct Register Register;

struct Register {
	Register *next;
	uint32_t reg;
	uint32_t value;
	int state;
	struct Timer timer;
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
	uint32_t regidx = 0;
	char op 	= 0;
	int32_t value 	= 0;
	uint64_t us 	= 0;  // timestamp added by sender, currently unused

	if (!ebus_parse_rsp(line, &op, &serial, &regidx, &value, &us))
		return 0;

	Device* dev = NULL;
	for (dev = bus->devices; dev; dev = dev->next)
		if (dev->serial == serial)
			break;

	if(!dev) return 0;

	Register* reg = NULL;
	for (reg = dev->registers; reg; reg = reg->next)
		if (reg->reg == regidx)
			break;

	if (!reg) return 0;

	int prev = reg->state;

	if (op == '=') {
		reg->value = value;
		reg->state = VALID;
	} else {
		reg->state = INVALID;
	}

	if (prev != PENDING)  // we got a response to someone elses request
		return 1;

	assert(timer_running(&reg->timer));
	int64_t dt = timer_tick_now(&reg->timer, 0);
	if (dt < 2) dt = 2;
	return dt;
}

enum {
	REQ_TIMEOUT_US = 1*1000*1000,   // after 1 second, PENDING->INVALID so next will re-issue
	RSP_TIMEOUT_US = 5*1000*1000    // after 5 seconds VALID->INVALID so next will re-issue
};

// Return number of timed out pending requests (not responses)
int bus_expire(Bus* bus) {
	Device* dev;
	Register* reg;
	int r = 0;
	int64_t t = now_us();
	int64_t s = 0;
	for (dev = bus->devices; dev; dev = dev->next)
		for (reg = dev->registers; reg; reg = reg->next) {
			switch (reg->state) {
			case PENDING:
				assert(timer_running(&reg->timer));
				s = t - timer_started(&reg->timer);
				if (0 < s && s < REQ_TIMEOUT_US)
					continue;
				timer_tick(&reg->timer, t, 0);
				reg->state = INVALID;
				++r;
				break;
			case VALID:
				assert(!timer_running(&reg->timer));
				s = t - timer_stopped(&reg->timer);
				if (0 < s && s < RSP_TIMEOUT_US)
					continue;
				reg->state = INVALID;
				break;
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
	memset(reg, 0, sizeof *reg);
	reg->reg = regidx;
	reg->state = INVALID;
	reg->next = dev->registers;
	dev->registers = reg;
	return reg;
}

int device_get_register(Device* dev, uint32_t regidx, uint32_t* val) {
	Register* reg = find_reg(dev, regidx);
	int64_t now;

	if (reg->state == VALID) {
		*val = reg->value;
		return 1;
	}

	if (reg->state == INVALID) {
		now = now_us();
		timer_tick(&reg->timer, now, 1);
		reg->state = PENDING;
		if (dev->bus->timestamp)
			fprintf(dev->bus->ctl, EBUS_GET_T_OFMT(dev->serial, regidx, now));
		else
			fprintf(dev->bus->ctl, EBUS_GET_OFMT(dev->serial, regidx));
		fflush(dev->bus->ctl);  // setlinebuf is unreliable in uclibc
	}

	return 0;
}

int device_set_register(Device* dev, uint32_t regidx, uint32_t val) {
	Register* reg = find_reg(dev, regidx);

	if (reg->state == PENDING && reg->value == val)
		return 0;

	if (reg->state == VALID && reg->value == val)
		return 1;

	// we were invalid, or pending or valid with a differend val

	const int64_t now = now_us();
	if (reg->state == PENDING)
		timer_tick(&reg->timer, now, 0);

	timer_tick(&reg->timer, now, 1);
	reg->state = PENDING;
	reg->value = val;

	if (dev->bus->timestamp)
		fprintf(dev->bus->ctl, EBUS_SET_T_OFMT(dev->serial, regidx, val, now));
	else
		fprintf(dev->bus->ctl, EBUS_SET_OFMT(dev->serial, regidx, val));
	fflush(dev->bus->ctl);   // setlinebuf is unreliable in uclibc
	return 0;
}

void device_invalidate_register(Device* dev, uint32_t regidx) {
	Register* reg;
	const int64_t now = now_us();
	for (reg = dev->registers; reg; reg = reg->next)
		if (reg->reg == regidx) {
			if (reg->state == PENDING)
				timer_tick(&reg->timer, now, 0);
			reg->state = INVALID;
			break;
		}
}

void device_invalidate_all(Device* dev) {
	Register* reg;
	const int64_t now = now_us();
	for (reg = dev->registers; reg; reg = reg->next) {
		if (reg->state == PENDING)
			timer_tick(&reg->timer, now, 0);
		reg->state = INVALID;
	}
}
