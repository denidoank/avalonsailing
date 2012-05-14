// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#ifdef __cplusplus
extern "C" {
#endif

// This library provides a line buffer that plays well with select(2).
// A zeroed structure is ready for use.  A LineBuffer can be fed a
// stream of characters and will hand them out in complete lines.  A
// line is complete if it ends with an EOL character ('\n').  When fed
// a stream of characters longer than the buffer size without seeing
// an EOL character, it will discard the current line until it sees an
// EOL again.
// Conventions:
//       read:  to LineBuffer from fd/char*
//       write: to fd/char*   from LineBuffer
// Argument order is like memmove: dst, src
struct LineBuffer {
	char line[1024];
	int head;    // next character to write to line
	int eol;     // one past position of first \n in line
	int discard; // if true, writestr and writefd should skip until the next eol.
};

// Pending returns true iff there is at least one complete line in the buffer.
int lb_pending(struct LineBuffer* lb);

// Readfd reads a stream of characters from fd into the linebuffer.
// If read(2) returns EOF or an error, readfd returns EOF or the
// errno, otherwise, if there is at least one complete line, it
// returns 0, otherwise it returns EAGAIN.  If the buffer overflows
// before EOL, this sets the discard flag which will cause the current
// line to be discarded until an EOL is found.
int lb_readfd(struct LineBuffer* lb, int fd);

// Readstr reads a stream of characters from **buf and update *buf
// according to the number of characters consumed. Returns 0 if there
// is a complete line or EAGAIN if not.
int lb_readstr(struct LineBuffer* lb, char** buf, int len);

// Writefd tries to write the first complete line to fd.  Returns 0 if
// no complete lines remain, EAGAIN if the first line was not
// completely written or more complete lines remain, or errno from
// write(2) otherwise.
int lb_writefd(int fd, struct LineBuffer* lb);

  // Same as writefd, but tries to write all complete lines in one write(2) call
int lb_writefd_all(int fd, struct LineBuffer* lb);

// Writestr tries to write the first complete line to to **buf and
// update *buf according to the number of characters written.  Returns
// 0 if no complete lines remain, or EAGAIN if not.
int lb_writestr(char **buf, int size, struct LineBuffer* lb);

int lb_writestr_all(char **buf, int size, struct LineBuffer* lb);

// Putline writes the zero terminated line in buf to lb.
// If buf does not end with a \n, it will write an extra one.
// If the buffer does not have space for the complete line,
// lb is unmodified and the function returns -1, otherwise
// it returs the number of characters copied from buf;
int lb_putline(struct LineBuffer* lb, char *buf);

// Getline copies the first complete line from lb to buf, and adds
// terminating zero.  If the provided buffer is too small buf and lb
// are unchanged and getline returns -1, otherwise it returns the
// number of characters copied (excluding the terminating zero), which
// will be 0 if there is no available complete line.
int lb_getline(char *buf, int size, struct LineBuffer* lb);


#ifdef __cplusplus
}
#endif
