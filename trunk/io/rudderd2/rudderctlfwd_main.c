#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
	const char* pfx = "rudderctl";
	const int n = strlen(pfx);
	setlinebuf(stdout);
	for(;;) {
		char line[1024];
		if (!fgets(line, sizeof(line), stdin)) exit(1);
		if (strncmp(line, pfx, n) != 0)	continue;
		if (fputc('#', stdout) == EOF) exit(1);
		if (fputs(line, stdout) == EOF) exit(1);
	}
	exit(1);
}
