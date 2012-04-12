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
#include <unistd.h>

struct Source {
	struct Source *next;
	int pid, lasti;
	int64_t dropped;
} *sources = NULL;


int main(int argc, char* argv[]) {

	if (argc > 1) {
		fprintf(stderr, "Usage: plug -o /path/to/lbus | loadtestrecv\n");
		exit(1);
	}

	int64_t dropped = 0;
	int64_t n = 0;
	int64_t x = 0;
	int64_t xx = 0;

	while(!feof(stdin)) {
		char line[1024];
		if (!fgets(line, sizeof line, stdin))
			break;

		struct timeval tv = { 0, 0 };
		gettimeofday(&tv, NULL);
		int pid, s, u, i;
		if (sscanf(line, "pid:%d timestamp_s:%d.%d seqno:%d\n", &pid, &s, &u, &i) != 4)
			break;
		
		int64_t diff_us = (tv.tv_sec-s)*1E6;
		diff_us += tv.tv_usec-u;

		n+=1;
		x += diff_us;
		xx += diff_us*diff_us;
		
		struct Source* src;
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

		if(src->lasti+1 != i) {
			dropped += i-src->lasti;
			src->dropped += i-src->lasti;
		}	
		src->lasti = i;
	}

	double a = x;  a/= n;
	double s = xx; s/= n;  s -= a*a; s=sqrt(s);

	printf("\nCount: %lld  avg:%f stdev:%f dropped:%lld\n", n, a, s, dropped);

	return 0;
}
