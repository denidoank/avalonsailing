/*
 *
./linebusd /tmp/lbus
./loadtestsend 10  | ./plug /tmp/lbus > /dev/null &
./loadtestsend 10  | ./plug /tmp/lbus > /dev/null &
./loadtestsend 10  | ./plug /tmp/lbus > /dev/null &
./loadtestsend 10  | ./plug /tmp/lbus > /dev/null &
./plug -o /tmp/lbus | ./loadtestrecv &
./plug -o /tmp/lbus | ./loadtestrecv &
./plug -o /tmp/lbus | ./loadtestrecv &
./plug -o /tmp/lbus | ./loadtestrecv &
# wait a bit
killall linebusd

Count: 13637  avg:482.678522 stdev:249.659719 dropped:0
Count: 13637  avg:497.579453 stdev:254.740223 dropped:0
Count: 13637  avg:493.812715 stdev:249.754643 dropped:0
....
output is in microseconds

 */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include "lib/timer.h"

struct Source {
	struct Source *next;
	int pid, lasti;
	int64_t dropped;
	struct Timer timer;
} *sources = NULL;


int main(int argc, char* argv[]) {

	if (argc > 1) {
		fprintf(stderr, "Usage: plug -o /path/to/lbus | loadtestrecv\n");
		exit(1);
	}

	int64_t dropped = 0;

	struct Timer timer;
	memset(&timer, 0, sizeof timer);

	struct Source* src;

	while(!feof(stdin)) {
		char line[1024];
		if (!fgets(line, sizeof line, stdin))
			break;

		int64_t now = now_us();

		int pid, s, u, i;
		if (sscanf(line, "pid:%d timestamp_s:%d.%d seqno:%d\n", &pid, &s, &u, &i) != 4)
			break;

		int64_t start_us = s * 1E6 + u;
		timer_tick(&timer, start_us, 1);
		timer_tick(&timer, now, 0);

		for(src = sources; src; src = src->next)
			if(src->pid == pid)
				break;
		if(!src) {
			src = malloc(sizeof *src);
			src->pid = pid;
			src->lasti = i - 1;
			src->dropped = 0;
			src->next = sources;
			sources = src;
		}

		timer_tick(&src->timer, start_us, 1);
		timer_tick(&src->timer, now, 0);

		if(src->lasti+1 != i) {
			dropped += i-src->lasti;
			src->dropped += i-src->lasti;
		}	
		src->lasti = i;
	}

	struct TimerStats stats;
	if(timer_stats(&timer, &stats)) {
		fprintf(stderr, "global count:%lld, dropped %lld  not enough events.\n", stats.count, dropped);
		return 0;
	}

	fprintf(stderr, "global " OFMT_TIMER_STATS(stats));
	fprintf(stderr, " dropped:%lld\n", dropped);
	fprintf(stderr, "Per client:\n");
	for(src = sources; src; src = src->next) {
		fprintf(stderr, "\tpid:%d ", src->pid);
		if(timer_stats(&src->timer, &stats)) {
			fprintf(stderr, "global count:%lld, dropped %lld  not enough events.", stats.count, dropped);
		} else {
			fprintf(stderr, OFMT_TIMER_STATS(stats));
			fprintf(stderr, " dropped:%lld\n", src->dropped);
		}
	}
	fprintf(stderr, "---\n");
	return 0;
}
