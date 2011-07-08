// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Methods for sequencing writes and reads to EPOS control registers.
//
// TODO: keep global read/write/error counts for health monitoring.
//

#include "seq.h"
#include "com.h"

#include <unistd.h>
#include <sys/time.h>

uint32_t
epos_sequence(int fd, uint8_t nodeid, struct EposCmd** cmd)
{
        uint32_t err = 0;
        int allfaults = 0;
        for (; (*cmd)->index && (allfaults < 10); ++(*cmd)) {
                struct EposCmd* c = *cmd;
                int delay = 10000; // 10ms
                int trys;
                for (trys = 0; trys < 3; ++trys, ++allfaults) {
                        err = epos_writeobject(fd, c->index, c->subindex, nodeid, c->value);
                        if (!err) break;
                        usleep(delay);
                        delay <<= 1;
                }
        }
        return err;
}

static int64_t
now_ms() 
{
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) crash("no working clock");

        int64_t ms1 = tv.tv_sec;  ms1 *= 1000;
        int64_t ms2 = tv.tv_usec; ms2 /= 1000;
        return ms1 + ms2;
}

uint32_t
epos_waitobject(int fd, int timeout_ms,
                uint16_t index, uint8_t subindex, uint8_t nodeid,
                uint32_t mask, uint32_t* value)
{
        long long int now = now_ms();
        if (now < 0) return 0x08000000;  // general error
        long long int deadline = now + timeout_ms;
        uint32_t err = 0;
        int trys;
        int delay2_us = 50*1000; // 50ms
        for (;;) {
                uint32_t val = 0;
                int delay1_us = 10*1000; // 50ms
                for (trys = 0; trys < 3; ++trys) {
                        err = epos_readobject(fd, index, subindex, nodeid, &val);
                        if (!err) break;
                        usleep(delay1_us);
                        delay1_us <<=1;
                }

                if (!err && (*value == (val & mask))) {
                        *value = val;
                        return 0;
                }

                if (deadline < now_ms()) return EPOS_ERR_TIMEOUT;
                usleep(delay2_us);
                delay1_us <<=1;
                if (delay2_us > 1000*1000) delay1_us = 1000*1000;
        }

        return 0;  // not reached
}
