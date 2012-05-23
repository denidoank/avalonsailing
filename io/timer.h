// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Statistics keeping start-stop timer.  Keeps running average
// period, duty cycle and max/min period and up over the last 64
// (TIMER_EVENTS) cycles.
//
//
#ifndef _IO_TIMER_H
#define _IO_TIMER_H

#include <stdint.h>


enum { TIMER_EVENTS = 64 };  // number of events to keep in timer ringbuffer

struct Timer {
	int64_t ticks[2*TIMER_EVENTS];  // starts(even) and stops(odd) ringbuffer
	int64_t count;		        // even: timer stopped, odd timer running.
	int ignoredups;		        // if set, repeated starts and stops are ignored.
};

int64_t now_us();  // current time in microseconds.  calls gettimeofday(2).

enum { TIMER_START = 1, TIMER_STOP = 0 /*, TIMER_RESTART = 2 */ };

// Register that the timer started (start == TIMER_START), stopped
// (start == TIMER_STOP) or restarted (start == TIMER_RESTART) at now_us.
// repeated calls with the same value of start are ignored (t->ignoredup is true)
// or cause the current event to be updated (default).
// Returns the number of microseconds since last start.
int64_t timer_tick(struct Timer* t,  int64_t now, int start);

// equivalent to timer_tick(t, now_us(), start)
int64_t timer_tick_now(struct Timer* t, int start);

// returns 1 if the timer is currently running.
int timer_running(struct Timer* t);

// returns the time the timer last started if running, or 0 if not running.
int64_t timer_started(struct Timer* t);
// returns the time the timer last stopped if not running, or 0 running.
int64_t timer_stopped(struct Timer* t);

// clear all events from the timer (by resetting count)
void timer_reset(struct Timer* t);

struct TimerStats {
	uint64_t count;
	double pmin, pavg, pdev, pmax;
	double rmin, ravg, rdev, rmax;
	double f, davg;
};

// Compute the timer stats over the ringbuffer.
// Count is the total number of events timed. the stats are only over the last TIMER_EVENTS.
// On succes, returns zero. If the timer is currently running or has
// fewer than 2 starts, only return count and return 1.
int timer_stats(struct Timer*t, struct TimerStats *s);

#define OFMT_TIMER_STATS(s) \
	"count:%lld   f(Hz): %.3lf dc(%%): %.1lf  period(ms): %.3lf / %.3lf (±%.3lf) / %.3lf  run(ms): %.3lf / %.3lf (±%.3lf) / %.3lf", \
		(s).count, (s).f, (s).davg*100, \
		(s).pmin/1000, (s).pavg/1000, (s).pdev/1000, (s).pmax/1000, \
		(s).rmin/1000, (s).ravg/1000, (s).rdev/1000, (s).rmax/1000


#endif // _IO_TIMER_H
