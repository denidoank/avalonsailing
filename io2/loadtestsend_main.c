
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

	if (argc < 2) {
		fprintf(stderr, "Usage: loadtestend $hz | plug -i /path/to/lbus\n");
		exit(1);
	}

	int hz = atoi(argv[1]);

	int pid = getpid();

	if(setvbuf(stdout, NULL, _IOLBF, 0))
		fprintf(stderr, "Failed to make stdout line-buffered.");

	int i = 0;

	for (;;) {
		struct timeval tv = { 0, 0 };
		gettimeofday(&tv, NULL);
		int s =  tv.tv_sec;
		int u =  tv.tv_usec;
		printf("pid:%d timestamp_s:%d.%06d seqno:%d\n", pid, s, u, i++);
		
		usleep(1E6/hz);
	}

	return 0;
}
