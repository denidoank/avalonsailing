// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Methods for sequencing writes and reads to EPOS control registers.
//

#include <stdint.h>

struct EposCmd {
        uint32_t index;
        uint8_t  subindex;
        uint32_t value;
};

/*
 * epos_sequence: send the commands listed in *cmd to the port using
 * epos_writeobject(), retrying a fixed number of times with a growing
 * backoff time.  On success advances cmd and repeats.  Processing
 * stops when (*cmd)->index is zero (end of table sentinel), when one
 * specific command has failed 3 times, or when the total number of
 * failures has reached 10.  Returns the last error and leaves cmd
 * pointing to the offending command or the sentinel record.
 */
uint32_t epos_sequence(int fd, uint8_t nodeid, struct EposCmd** cmd);

/*
 * Repeatedly tries to read the specified register, retrying up to 3
 * times on failure with an increasing backoff.  Exits when the read
 * value & mask equals the specified *value or when timeout_ms
 * milliseconds have passed.  Writes the actual value to *value, and
 * returns the last readobject's error code.
 * Note that this effectively blocks for timeout_ms. use with care.
 */
uint32_t epos_waitobject(int fd, int timeout_ms,
                         uint16_t index, uint8_t subindex, uint8_t nodeid,
                         uint32_t mask, uint32_t* value);
