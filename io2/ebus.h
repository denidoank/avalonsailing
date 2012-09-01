// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// API for the ebus protocol
//
#ifndef _IO_RUDDERD2_EBUS_H
#define _IO_RUDDERD2_EUBS_H

#include <stdint.h>

// All (uint16_t index , uint8_t subindex)  are folded into an uint_32 in this api.
#define REGISTER(index, subindex) (((index) << 8) | ((subindex)&0xff))

#define INDEX(reg) (((reg)>>8) & 0xffff)
#define SUBINDEX(reg) ((reg) & 0xff)


// These define the protocol understood and emitted by eposcom and clients
#define EBUS_GET_OFMT(serial, reg) 	  "0x%x:0x%x[%d]\n",         (serial), INDEX(reg), SUBINDEX(reg)
#define EBUS_SET_OFMT(serial, reg, value) "0x%x:0x%x[%d] := 0x%x\n", (serial), INDEX(reg), SUBINDEX(reg), (value)
#define EBUS_ACK_OFMT(serial, reg, value) "0x%x:0x%x[%d] = 0x%x\n",  (serial), INDEX(reg), SUBINDEX(reg), (value)
#define EBUS_ERR_OFMT(serial, reg, err)   "0x%x:0x%x[%d] # 0x%x\n",  (serial), INDEX(reg), SUBINDEX(reg), (err)

// for use with timestamps
#define EBUS_GET_T_OFMT(serial, reg, us)         "0x%x:0x%x[%d] ? T:%lld\n",       (serial), INDEX(reg), SUBINDEX(reg), (us)
#define EBUS_SET_T_OFMT(serial, reg, value, us)	 "0x%x:0x%x[%d] := 0x%x T:%lld\n", (serial), INDEX(reg), SUBINDEX(reg), (value), (us)
#define EBUS_ACK_T_OFMT(serial, reg, value, us)  "0x%x:0x%x[%d] = 0x%x T:%lld\n",  (serial), INDEX(reg), SUBINDEX(reg), (value), (us)
#define EBUS_ERR_T_OFMT(serial, reg, err,   us)  "0x%x:0x%x[%d] # 0x%x T:%lld\n",  (serial), INDEX(reg), SUBINDEX(reg), (err), (us)

// Parses request (SET/GET[_T]_OFMT) lines.  us not set if T: missing.  sets op to '?' or ':'
// Returns 1 if the line contained a request, 0 otherwise.
int ebus_parse_req(const char* line, char* op, uint32_t* serial, uint32_t* reg, int32_t* val, uint64_t* us);

// Parses response (ACK/ERR[_T]_OFMT) lines.  us not set if T: missing.  sets op to '=' or '#'
// Returns 1 if the line contained a response, 0 otherwise.
int ebus_parse_rsp(const char* line, char* op, uint32_t* serial, uint32_t* reg, int32_t* val, uint64_t* us);

// Parses any request or response
// Returns 1 if the line contained a response or request, 0 otherwise.
int ebus_parse(const char* line, char* op, uint32_t* serial, uint32_t* reg, int32_t* val, uint64_t* us);

#endif //_IO_RUDDERD2_EBUS_H
