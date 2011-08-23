// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "linebuffer.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Reads from fd into the linebuffer, returns 0 if there is a \n terminated
// line, EAGAIN if there is an incomplete line, or errno from read(2)
// otherwise.  if the buffer overflows before EOL sets the discard flag which will
// cause the current line to be discarded until an EOL is found.
// the line will be 0-terminated.
int lb_read(int fd, struct LineBuffer* lb) {
	int n = read(fd, lb->line+lb->head, sizeof(lb->line) - lb->head - 1);
	switch (n) {
	case 0:
		return EOF;
	case -1:
		return errno;
	}

	lb->line[lb->head+n] = 0;
	char* eol = strchr(lb->line+lb->head, '\n');
	if (eol) {
		lb->head = 0;
		if (!lb->discard) return 0;  // lb->line starts with a complete line
		lb->discard = 0;
	}
	
	// no EOL || discard
	lb->head += n;
	if (lb->head == sizeof(lb->line) - 1) lb->discard = 1;
	if (lb->discard) lb->head = 0;
	return EAGAIN;
}

void lb_putline(struct LineBuffer* lb, const char* line) {
	strncpy(lb->line, line, sizeof lb->line - 1);
	lb->line[sizeof lb->line - 1] = 0;
	lb->head = strlen(lb->line);
}

int lb_pending(struct LineBuffer* lb) {
	return lb->head > 0;
}

int lb_write(int fd, struct LineBuffer* lb) {
	int n = write(fd, lb->line + lb->tail, lb->head - lb->tail);
	if (n < 0) return errno;
	lb->tail += n;
	if (lb->tail != lb->head) return EAGAIN;
	lb->head = 0;
	lb->tail = 0;
	return 0;
}
