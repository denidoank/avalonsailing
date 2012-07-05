// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
#ifndef _IO_RUDDERD2_EPOSCLIENT_H
#define _IO_RUDDERD2_EPOSCLIENT_H

#include <stdint.h>
#include <stdio.h>

// A bus is a read/write-through cache of register values, grouped per
// device.  Reads and writes that can't be satisfied Result in
// requests on the bus' backing FILE*. (connected to an ebus).
typedef struct Bus Bus;
typedef struct Device Device;

// Instantiate and connect a new bus.  ctl will be used to issue commands.
Bus* bus_new(FILE* ctl);

// set or clear the add-timestamps-to-generated-messages flag.
void bus_enable_timestamp(Bus* bus, int on);

// parse and dispatch a received line to devices.
// returns 0 if no register updated, or the time difference in uS since the command was issued.
// If the received message was a response to someone elses request, a fake time difference of 1 uS is reported.
int64_t bus_receive(Bus* bus, char* line);

// Timeout variables to the INVALID state that have been PENDING for more than 1 second
// and clear cached VALID values that are older than 5 seconds.
// Returns the number of pending requests timed out.
int bus_expire(Bus* bus);

// Connect a new device with serial number @serial to the bus. repeated opens are idempotent.
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
// will always result in an request to be transmitted on the bus.
void device_invalidate_register(Device* dev, uint32_t reg);
void device_invalidate_all(Device* dev);

#endif //_IO_RUDDERD2_EPOSCLIENT_H
