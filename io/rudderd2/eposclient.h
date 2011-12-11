// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Public api of a library to talk to an eposcom 
//
#ifndef _IO_RUDDERD2_EPOSCLIENT_H
#define _IO_RUDDERD2_EPOSCLIENT_H

#include <stdint.h>
#include <stdio.h>

// All (uint16_t index , uint8_t subindex)  are folded into an uint_32 in this api.
#define REGISTER(index, subindex) (((index) << 8) | ((subindex)&0xff))

#define INDEX(reg) (((reg)>>8) & 0xffff)
#define SUBINDEX(reg) ((reg) & 0xff)

#define EBUS_GET_OFMT(serial, reg) \
	"0x%x:0x%x[%d]\n", (serial), INDEX(reg), SUBINDEX(reg)

#define EBUS_SET_OFMT(serial, reg, value) \
	"0x%x:0x%x[%d] := 0x%x\n", (serial), INDEX(reg), SUBINDEX(reg), (value)


typedef struct Bus Bus;
typedef struct Device Device;

// Instantiate and connect a new bus.  ctl will be used to issue commands.
Bus* bus_new(FILE* ctl);

// parse and dispatch a received line to devices.
int bus_receive(Bus* bus, char* line);

// Timeout variables to the INVALID state that have been PENDING for more than 1 second.
void bus_clocktick(Bus* bus);

// Connect a new device to the bus. repeated opens are idempotent.
Device* bus_open_device(Bus* bus, uint32_t serial);

// If the register is VALID put the value immediately in val and return true
// otherwise, send an instruction to get the register on the bus, except
// when such an instruction is already PENDING.
int  device_get_register(Device* dev, uint32_t reg, uint32_t* val);

// If the register is VALID and has the same value already, return true
// immediately, otherwise send and instruction to set the register
// on the bus, except when such an instruction is already PENDING *for
// the same value*.
int  device_set_register(Device* dev, uint32_t reg, uint32_t val);

// Force the register state to be INVALID so next get or set
// will always result in an instruction to be transmitted on the bus.
void device_invalidate_register(Device* dev, uint32_t reg);

void device_invalidate_all(Device* dev);

#endif //_IO_RUDDERD2_EPOSCLIENT_H
