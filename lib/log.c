// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "log.h"

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

void crash(const char* fmt, ...) {
	va_list ap;
	char buf[1000];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	syslog(LOG_CRIT, "%s%s%s\n", buf,
	       (errno) ? ": " : "",
	       (errno) ? strerror(errno):"" );
	exit(1);
	va_end(ap);
	return;
}


void fault(int i) { crash("fault"); }

static int64_t now_ms() {
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) crash("no working clock");

        int64_t ms1 = tv.tv_sec;  ms1 *= 1000;
        int64_t ms2 = tv.tv_usec; ms2 /= 1000;
        return ms1 + ms2;
}

struct TokenBucket {
	int size;  // burst size
	int rate;  // max per second
	int val;   // current value
	int discarded;  // discarded since last limiting 
	int64_t last_ms;  // time of last 
} buckets[] = {
	[LOG_EMERG]   = { 100, 10, 0, 0 },
	[LOG_ALERT]   = { 100, 10, 0, 0 },
	[LOG_CRIT]    = { 100, 10, 0, 0 },
	[LOG_ERR]     = { 10, 10, 0, 0 },
	[LOG_WARNING] = { 10, 10, 0, 0 },
	[LOG_NOTICE]  = { 10, 10, 0, 0 },
	[LOG_INFO]    = { 10, 10, 0, 0 },
	[LOG_DEBUG]   = { 10, 10, 0, 0 },
};

#define nelem(x) (sizeof(x)/sizeof(x[0]))

int min(int64_t x, int64_t y) { return (x < y) ? x : y; }

void slog(int priority, const char *message, ...) {
	if (priority < 0 || priority >= nelem(buckets)) crash("invalid priority");

	va_list ap;
	va_start(ap, message);

	struct TokenBucket* b = buckets + priority;
	int64_t now = now_ms();
	if (b->last_ms > now) b->last_ms = now;  // guard against clock jump;
	b->val = min(b->val + ((now - b->last_ms) * b->rate / 1000), b->size);

	if (b->val > 0) {
		if (b->discarded) {
			syslog(priority, "discarded %d log messages", b->discarded);
			b->discarded = 0;
		}
		b->val--;
		b->last_ms = now;
		vsyslog(priority, message, ap);
	} else {
		b->discarded++;
	}

	va_end(ap);
	return;
}
