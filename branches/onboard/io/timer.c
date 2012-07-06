// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
#include "timer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int64_t now_ms() { return now_us()/1000; }
int64_t now_us() {
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) {
                fprintf(stderr, "no working clock");
                exit(1);
        }

        int64_t us1 = tv.tv_sec;  us1 *= 1000000;
        int64_t us2 = tv.tv_usec;
        return us1 + us2;
}

// TODO handle restart 
int64_t timer_tick(struct Timer* t,  int64_t now, int start) {
	start = start ? 0 : 1;
	if (start == (t->count & 1) && t->ignoredups) return 0;
	int64_t last_start = now;
	if(t->count) last_start = t->ticks[ ((t->count-1) % (2*TIMER_EVENTS)) & ~1 ];
	t->ticks[((t->count++ % (2*TIMER_EVENTS)) & ~1) | start] = now;
	return now - last_start;
}

int64_t timer_tick_now(struct Timer* t, int start) { return timer_tick(t, now_us(), start); }
int timer_running(struct Timer* t) { return t->count & 1; }

int64_t timer_started(struct Timer* t) {
	if((t->count & 1) == 0)
		return 0;
	return t->ticks[((t->count-1) % (2*TIMER_EVENTS))];
}

int64_t timer_stopped(struct Timer* t) {
	if(((t->count & 1) != 0) || t->count < 2)
		return 0;
	return t->ticks[((t->count-1) % (2*TIMER_EVENTS))];
}

void timer_reset(struct Timer* t) { t->count = 0; }

int timer_stats(struct Timer*t, struct TimerStats *s) {
	memset(s, 0, sizeof *s);
	s->count = t->count / 2;
	if (s->count < 2) return 1;
	if(timer_running(t)) return 1;

	int nn = (s->count < TIMER_EVENTS) ? s->count : TIMER_EVENTS; // number of events in queue
	int last = (s->count - 1) % TIMER_EVENTS;  // last written event
	int first = (s->count < TIMER_EVENTS) ? 0 : (s->count % TIMER_EVENTS) ; // first event in queue

	int i, j;
	double sx, sxx;

	// average period is  last start - first start / (nn - 1)
	sx = t->ticks[2*last] - t->ticks[2*first];
	s->pavg = sx / (nn - 1);

	s->f = (s->pavg == 0) ? 0 : 1E6 / s->pavg;

	// min/average/max run time 
	sx = 0;
	s->rmin = HUGE_VAL; s->rmax = -HUGE_VAL;
	for(i = 0; i < nn; ++i) {
		double x = t->ticks[2*i + 1] - t->ticks[2*i];
		if (s->rmin > x) s->rmin = x;
		if (s->rmax < x) s->rmax = x;
		sx += x;
	}
	sx /= nn;
	s->ravg = sx;
	s->davg = (s->pavg == 0) ? 1 : s->ravg / s->pavg;

	sxx = 0;
	for(i = 0; i < nn; ++i) {
		double x = t->ticks[2*i + 1] - t->ticks[2*i];
		x -= sx;
		sxx += x*x;
	}
	s->rdev = sqrt(sxx/nn);

	sxx = 0;
	sx = s->pavg;
	for (i = 0; i < nn; ++i) {
		if (i == last) continue;     // don't count first-last as a period
		j = (i + 1) % TIMER_EVENTS;  // next

		double x = t->ticks[2*j] - t->ticks[2*i];      // period
		if (s->pmin > x) s->pmin = x;
		if (s->pmax < x) s->pmax = x;
		
		x -= sx;
		sxx += x*x;
	}
	s->pdev = sqrt(sxx/(nn-1));
	
	return 0;
}

