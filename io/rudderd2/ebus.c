// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "ebus.h"

#include <stdio.h>
// Parses request (SET/GET[_T]_OFMT) lines.  us not set if T: missing.  sets op to '?' or ':'
// Returns 1 if the line contained a request, 0 otherwise.
int ebus_parse_req(const char* line, char* op, uint32_t* serial, uint32_t* reg, int32_t* val, uint64_t* us) {
	return ebus_parse(line, op, serial, reg, val, us) && (*op == '?' || *op == ':');
}

// Parses response (ACK/ERR[_T]_OFMT) lines.  us not set if T: missing.  sets op to '=' or '#'
// Returns 1 if the line contained a response, 0 otherwise.
int ebus_parse_rsp(const char* line, char* op, uint32_t* serial, uint32_t* reg, int32_t* val, uint64_t* us) {
	return ebus_parse(line, op, serial, reg, val, us) && (*op == '=' || *op == '#');
}

// Parses any request or response
// Returns 1 if the line contained a response or request, 0 otherwise.
int ebus_parse(const char* line, char* op, uint32_t* serial, uint32_t* reg, int32_t* val, uint64_t* us) {

	int index       = 0;
	int subindex    = 0;
	char ops[3] 	= { 0, 0, 0 };
	int64_t value_l = 0;
	int nn 		= 0;
	int n = sscanf(line, "%i:%i[%i] %2s %n%lli T:%lld", serial, &index, &subindex, ops, &nn, &value_l, us);

	if(n < 3) return 0;

	*reg = REGISTER(index, subindex);

	switch (n) {
	case 3:		// get short form (without timestamp)
		*op = '?';
		return 1;
	case 4: 
		if (ops[0] == '?') {  // get longer form with optional timestamp
			*op = '?';
			sscanf(line+nn, "T:%lld", us);
			return 1;
		}
		return 0;
	case 5:
	case 6:
		*op = ops[0];
		*val = value_l;
		return 1;
	}
		
	return 0;
}
