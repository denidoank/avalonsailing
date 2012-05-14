#include "linebuffer.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {

	struct LineBuffer lb;
	memset(&lb, 0, sizeof lb);
	
	assert(!lb_pending(&lb));

	char * line = "incomplete";
	
	assert(lb_readstr(&lb, &line, strlen(line)) == EAGAIN);
	assert(*line == 0);

	char out[100] = { 1, 1, 0, };
	assert(lb_getline(out, sizeof out, &lb) == 0);
	assert(out[0] == 1 && out[1] == 1 && out[2] == 0);
	
	line = "finish\n";
	assert(lb_readstr(&lb, &line, strlen(line)) == 0);

	assert(lb_getline(out, sizeof out, &lb) == strlen("incompletefinish\n"));
	assert(!strcmp(out, "incompletefinish\n"));
	assert(!lb_pending(&lb));

	assert(lb_putline(&lb, "one") == 3);   // adds \n
	assert(lb_pending(&lb));
	assert(lb_putline(&lb, "two\n") == 4);
	assert(lb_pending(&lb));
	assert(lb_putline(&lb, "three") == 5);     // adds \n

	line = out;
	assert(lb_writestr(&line, sizeof out, &lb) == EAGAIN);
	*line = 0;
	assert(!strcmp(out, "one\n"));
	line = out;
	assert(lb_writestr_all(&line, sizeof out, &lb) == 0);
	*line = 0;
	assert(!strcmp(out, "two\nthree\n"));

	// discarding

	int i, n;
	n = strlen("incomplete");
	for (i=0; i < 1024; i += n) {
		assert(!lb.discard);
		line = "incomplete";
		assert(lb_readstr(&lb, &line, n) == EAGAIN);
	}
	assert(lb.discard);
	assert(!lb_pending(&lb));
	assert(lb_putline(&lb, "one") == 3);   // adds \n, terminates discard
	assert(!lb_pending(&lb)); 
	assert(!lb.discard);
	assert(lb_putline(&lb, "two\n") == 4);
	assert(lb_pending(&lb));

	line = out;
	assert(lb_writestr_all(&line, sizeof out, &lb) == 0);
	*line = 0;
	assert(!strcmp(out, "two\n"));
	assert(lb.head == 0); 

	// boundary cases
	assert(lb_putline(&lb, "") == 0);   // putline does not add empty line
	assert(lb.head == 0);
	assert(!lb_pending(&lb)); 

	assert(lb_putline(&lb, "\n") == 1);   // putline will add a single \n
	assert(lb.head == 1);
	assert(lb_pending(&lb)); 
	line = out;
	assert(lb_writestr(&line, sizeof out, &lb) == 0);
	*line = 0;

	assert(!strcmp(out, "\n"));

	puts("OK");
	return 0;
}
