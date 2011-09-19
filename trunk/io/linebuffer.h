// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#ifdef __cplusplus
extern "C" {
#endif

// line buffer that plays well with select
struct LineBuffer {
	char line[1024];
	int head;
	int discard;
	int tail;
};

// Reads from fd into the linebuffer, returns 0 if there is a \n terminated
// line, EAGAIN if there is an incomplete line, or errno from read(2)
// otherwise.  if the buffer overflows before EOL sets the discard flag which will
// cause the current line to be discarded until an EOL is found.
// the line will be 0-terminated.
int lb_read(int fd, struct LineBuffer* lb);

// Writing, call lb_putline and then lb_write until it returns 0
void lb_putline(struct LineBuffer* lb, const char* line);
int lb_pending(struct LineBuffer* lb);
int lb_write(int fd, struct LineBuffer* lb);

#ifdef __cplusplus
}
#endif
