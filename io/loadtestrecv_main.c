/*
 *
./linebusd /tmp/lbus &
./loadtestsend 10  | ./plug -i /tmp/lbus &
./loadtestsend 10  | ./plug -i /tmp/lbus &
./loadtestsend 10  | ./plug -i /tmp/lbus &
./loadtestsend 10  | ./plug -i /tmp/lbus &
./plug -o /tmp/lbus | ./loadtestrecv &
./plug -o /tmp/lbus | ./loadtestrecv &
./plug -o /tmp/lbus | ./loadtestrecv &
./plug -o /tmp/lbus | ./loadtestrecv &
# wait a bit
killall linebusd

Count: 781  avg:281.353393 stdev:19.582795
Count: 741  avg:256.026991 stdev:15.392808
Count: 821  avg:280.797808 stdev:19.718866
Count: 865  avg:293.870520 stdev:32.068992

output is in microseconds

 */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

	if (argc > 1) {
		fprintf(stderr, "Usage: plug -o /path/to/lbus | loadtestrecv\n");
		exit(1);
	}

	int64_t n = 0;
	int64_t x = 0;
	int64_t xx = 0;

	while(!feof(stdin)) {
		char line[1024];
		if (!fgets(line, sizeof line, stdin))
			break;

		struct timeval tv = { 0, 0 };
		gettimeofday(&tv, NULL);
		int pid, s, u;
		if (sscanf(line, "pid:%d timestamp_s:%d.%d\n", &pid, &s, &u) != 3)
			break;
		
		int64_t diff_us = (tv.tv_sec-s)*1E6;
		diff_us += tv.tv_usec-u;

		n+=1;
		x += diff_us;
		xx += diff_us*diff_us;
	}

	double a = x;  a/= n;
	double s = xx; s/= n;  s -= a*a; s=sqrt(s);

	printf("Count: %lld  avg:%f stdev:%f\n", n, a,s);

	return 0;
}
