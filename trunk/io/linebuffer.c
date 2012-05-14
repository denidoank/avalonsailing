// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "linebuffer.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int lb_pending(struct LineBuffer* lb) { return lb->eol > 0; }

// like write(2), but to buf[0:size] instead of fd
static int write_buf(char *buf, int size, char *src, int len) {
	if(len > size) len = size;
	memmove(buf, src, len);
	return len;
}

// like read(2) but from buf[0:len] instead of fd
static int read_buf(char* buf, int len, char* dst, int size) {
	if (len > size) len = size;
	memmove(dst, buf, len);
	return len;
}

// -----------------------------------------------------------------------------
//    Reading
// -----------------------------------------------------------------------------

static int size_left(struct LineBuffer* lb) { return sizeof(lb->line) - lb->head - 1; }
static int update_r(struct LineBuffer* lb, int n);

int lb_readfd(struct LineBuffer* lb, int fd) {
	int n = read(fd, lb->line + lb->head, size_left(lb));
	if (n == 0)
		return EOF;
	if (n < 0)
		return errno;
	return update_r(lb, n);
}

int lb_readstr(struct LineBuffer* lb, char** buf, int len) {
	int n = read_buf(*buf, len, lb->line + lb->head, size_left(lb));
	*buf += n;
	return update_r(lb, n);
}

static int update_r(struct LineBuffer* lb, int n) {
//	printf("update_r before: head:%d eol:%d  n:%d\n", lb->head, lb->eol, n);
	int savehead = lb->head;
	lb->head += n;

	if(lb->eol == 0) {
		char *eol = memchr(lb->line + savehead, '\n', n);
		if (eol)
			lb->eol = eol - lb->line + 1;
	}

	if(lb->eol == 0  && lb->head  >= sizeof(lb->line) - 1) {
		lb->discard = 1;
	}

	if(lb->discard) {
		if(lb->eol == 0) 
			lb->eol = lb->head;
		else
			lb->discard = 0;
		memmove(lb->line, lb->line + lb->eol, lb->head - lb->eol);
		lb->head -= lb->eol;
		lb->eol = 0;
		char *eol = memchr(lb->line, '\n', lb->head);
		if (eol)
			lb->eol = eol - lb->line + 1;
	}
//	printf("update_r after: head:%d eol:%d  n:%d\n", lb->head, lb->eol, n);
	return lb->eol == 0 ? EAGAIN : 0;
}

static char EOL = '\n';

int lb_putline(struct LineBuffer* lb, char *buf) {
	int len = strlen(buf);
	int mustadd = (len == 0 || buf[len-1] == '\n') ? 0 : 1;
	if(len + mustadd > size_left(lb))
		return -1;
	lb_readstr(lb, &buf, len);
	char* c = &EOL;
	if(mustadd) lb_readstr(lb, &c, 1);
	return len;
}


// -----------------------------------------------------------------------------
//    Writing
// -----------------------------------------------------------------------------
static int update_w(struct LineBuffer* lb, int n);

int lb_writefd(int fd, struct LineBuffer* lb) {
	if(lb->eol == 0) return 0;
	int n = write(fd, lb->line, lb->eol);
	if (n < 0) return errno;
	return update_w(lb, n);
}

int lb_writestr(char **buf, int size, struct LineBuffer* lb) {
	if(lb->eol == 0) return 0;
	int n = write_buf(*buf, size, lb->line, lb->eol);
	*buf += n;
	return update_w(lb, n);
}

static int last_eol(struct LineBuffer* lb) {
	int eol = lb->eol;
	int remain = lb->head - eol;
	while(remain) {	// look for more lines
		char* last = memchr(lb->line + eol, '\n', remain);
		if (!last) break;
		eol = last + 1 - lb->line;
		remain = lb->head - eol;
	}
	return eol;
}

int lb_writefd_all(int fd, struct LineBuffer* lb) {
	if(lb->eol == 0) return 0;
	int eol = last_eol(lb);
	int n = write(fd, lb->line, eol);
	if (n < 0) return errno;
	lb->eol = n;   	// pretend this was the first one. so update clears it and looks for next.
	return update_w(lb, n);
}

int lb_writestr_all(char **buf, int size, struct LineBuffer* lb) {
	if(lb->eol == 0) return 0;
	int eol = last_eol(lb);
	int n = write_buf(*buf, size, lb->line, eol);
	*buf += n;
	lb->eol = n;   	// pretend this was the first one. so update clears it and looks for next.
	return update_w(lb, n);
}

static int update_w(struct LineBuffer* lb, int n) {
//	printf("update_w before: head:%d eol:%d  n:%d\n", lb->head, lb->eol, n);
	memmove(lb->line, lb->line+n, lb->head-n + 1);
	lb->eol -= n;
	lb->head -=n;
	if(lb->eol == 0) {
		char *eol = memchr(lb->line, '\n', lb->head);
		if (eol)
			lb->eol = eol - lb->line + 1;
	}
//	printf("update_w after: head:%d eol:%d  n:%d\n", lb->head, lb->eol, n);
	return lb->eol == 0 ? 0 : EAGAIN;
}

int lb_getline(char *buf, int size, struct LineBuffer* lb) {
	if(lb->eol == 0)
		return 0;
	int len = lb->eol+1;  // terminating zero
	if(size < len)
		return -1;
	lb_writestr(&buf, size, lb);
	*buf = 0;
	return len-1;
}
