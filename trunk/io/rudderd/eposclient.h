// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Public api of a library to talk to an eposcom process either directly
// or through eposd.
// 
#ifndef _IO_RUDDERD_EPOSCLIENT_H
#define _IO_RUDDERD_EPOSCLIENT_H

#include <stdint.h>
#include <sys/select.h>

typedef struct Bus Bus;
typedef struct Device Device;

// Instantiate and connect a new bus
Bus* bus_open_eposd(char* path_to_socket);
Bus* bus_open_eposcom(char* path_to_eposcom, char* path_to_port);

void bus_close(Bus* bus);   // close and free.

// setup fd_sets for select.  returns feof(bus->ctl) || feof(bus->sts)
int bus_set_fds(Bus* bus, fd_set* rfds, fd_set* wfds, int* maxfd);

// while !EAGAIN, receive lines from the eposd or eposcom sts fd, and dispatch to devices.
int bus_receive(Bus* bus, fd_set* rfds);

// Timeout variables to the INVALID state that have been PENDING for more than 1 second.
void bus_clocktick(Bus* bus);

// try to write any unwritten lines to the eposd or eposcom ctl fd
int bus_flush(Bus* bus, fd_set* wfds);

// Connect a new device to the bus. repeated opens are idempotent.
Device* bus_open_device(Bus* bus, uint32_t serial);
void bus_close_device(Device* dev);  // close and free

// All (uint16_t index , uint8_t subindex)  are folded into an uint_32 in this api.
#define REGISTER(index, subindex) (((index) << 8) | ((subindex)&0xff))

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

#endif //_IO_RUDDERD_EPOSCLIENT_H
