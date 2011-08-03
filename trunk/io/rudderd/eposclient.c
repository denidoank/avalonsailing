// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "eposclient.h"

#ifdef DEBUG
static void
vlogf(const char* fmt, ...)
{
        va_list ap;
        char buf[1000];
        va_start(ap, fmt);
        vsnprintf(buf, 1000, fmt, ap);
        fprintf(stderr, "%s%s%s\n", buf,
                (errno) ? ": " : "",
                (errno) ? strerror(errno): "" );
        va_end(ap);
        return;
}
#define VLOGF(...) vlogf(__VA_ARGS__)
#else
#define VLOGF(...) do {} while(0)
#endif

enum { INVALID, PENDING, VALID };

typedef struct Register Register;

struct Register {
	Register *next;
	uint32_t reg;
	uint32_t value;
	int state;
	long issued_ms;
};

struct Device {
	Device *next;
	Bus *bus;
	uint32_t serial;
	Register* registers;
};

struct Bus {
	FILE* sts;
	FILE* ctl;
	int pending;  // potentially unflushed data written to ctl's buffers
	Device* devices;
};


// ------------------------------------------------------------------------------

static Bus* bus_new(FILE* sts, FILE* ctl) {
	Bus* bus = malloc(sizeof(*bus));
        bus->sts = sts; 
        bus->ctl = ctl;
       if (fcntl(fileno(bus->sts),  F_SETFL, O_NONBLOCK) < 0) VLOGF("fcntl(in)");
//        if (fcntl(fileno(bus->ctl), F_SETFL, O_NONBLOCK) < 0) VLOGF("fcntl(out)");
        setlinebuf(bus->ctl); // 64k out buffer
        bus->pending = 0;
	bus->devices = NULL;
	return bus;
}


// Instantiate a new bus
Bus* bus_open_eposd(char* path_to_socket) {
        int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (fd < 0) {
		VLOGF("socket");
		return NULL;
	}
        struct sockaddr_un addr;
        addr.sun_family = AF_LOCAL;
        strncpy(addr.sun_path, path_to_socket, sizeof(addr.sun_path));
        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                VLOGF("connect(%s)", path_to_socket);
		close(fd);
		return NULL;
	}
	return bus_new(fdopen(fd, "r"), fdopen(dup(fd), "w"));
}

static pid_t
popen2(char* cmd, FILE** ctl, FILE**sts, char* arg)
{
	int p_stdin[2], p_stdout[2];  // 0,1 = read,write
	if (pipe(p_stdin) || pipe(p_stdout)) {
		VLOGF("pipe");
		return -1; 
	}
	pid_t pid = fork();
	if (pid < 0) {
		VLOGF("fork");
		return pid;
	}
	if (pid == 0) {		// child code
		close(p_stdin[1]);
		close(p_stdout[0]);
		dup2(p_stdin[0], 0);  // read end of p_stdin is my stdin
		dup2(p_stdout[1], 1);  // write end of p_stdout is my stdout
		execl(cmd, cmd, arg, (char*)NULL);
		VLOGF("execl(%s %s)",  cmd, arg);
		exit(1);
	}
	close(p_stdin[0]);
	close(p_stdout[1]);
	*ctl = fdopen(p_stdin[1],  "w");
	*sts = fdopen(p_stdout[0], "r");
	return pid;
}

Bus* bus_open_eposcom(char* path_to_eposcom, char* path_to_port) {
	FILE *ctl, *sts;
	if (popen2(path_to_eposcom, &ctl, &sts, path_to_port) < 0) {
		VLOGF("popen2(%s %s)", path_to_eposcom, path_to_port);
		return NULL;
	}
	return bus_new(sts, ctl);
}

void bus_close(Bus* bus) {
	fclose(bus->ctl);
	fclose(bus->sts);
	free(bus);
}

int bus_set_fds(Bus* bus, fd_set* rfds, fd_set* wfds, int* maxfd) {
        if (rfds && bus->sts && !feof(bus->sts)) {
		FD_SET(fileno(bus->sts), rfds);
		if (maxfd && (*maxfd < fileno(bus->sts))) *maxfd = fileno(bus->sts);
	}
	if (wfds && bus->ctl && bus->pending && !feof(bus->ctl)) {
		FD_SET(fileno(bus->ctl), wfds);
		if (maxfd && (*maxfd < fileno(bus->ctl))) *maxfd = fileno(bus->ctl);
	}
        return feof(bus->sts) || feof(bus->ctl);
}

// while !EAGAIN, receive lines from the eposd or eposcom sts fd, and dispatch to devices.
int bus_receive(Bus* bus, fd_set* rfds) {
	if (!FD_ISSET(fileno(bus->sts), rfds))
	    return EAGAIN;
	VLOGF("bus->sts has input");
	char line[1024];
	while (fgets(line, sizeof(line), bus->sts)) {
		if (line[0] == '#') {
			VLOGF("Comment: %s", line);
			continue;
		}

                uint32_t serial = 0;
		int index     = 0;
		int subindex  = 0;
		char op[3];   // := or =
		int64_t value = 0;  // fumble signdness
		int32_t error = 0;

		int n = sscanf(line, "%i:%i[%i] %2s %lli (%i)",
			       &serial, &index, &subindex, op, &value, &error);
		if (n != 6) {
			VLOGF("Unparsable line: %s", line);
			continue;
		}

		if ((strcmp(op, ":=") == 0) || (strcmp(op, "=") == 0)) {
			uint32_t idx = REGISTER(index, subindex);
			VLOGF("eposclient: response for register %x = %x:", idx, value, line);
			Device* dev;
			Register* reg;
			for (dev = bus->devices; dev; dev = dev->next)
				if (dev->serial == serial) {
					for (reg = dev->registers; reg; reg = reg->next)
						if (reg->reg == idx) {
							if (error == 0) {
								reg->value = value;
								reg->state = VALID;
							} else {
								reg->state = INVALID;
							}
							break;
						}
					break;
				}
		} else {
			VLOGF("Invalid operator: %s", line);
			continue;
		}
	}
	return feof(bus->sts) ? EOF : ferror(bus->sts);
}

int bus_flush(Bus* bus, fd_set* wfds) {
	if (!FD_ISSET(fileno(bus->ctl), wfds))
		return EAGAIN;
	int r = fflush(bus->ctl);
	VLOGF("fflushing %d instructions to bus->ctl:%d", bus->pending, r);
	if (r == 0) {
		bus->pending = 0;
		return 0;
	}
	return errno; 
}

#define INDEX(reg) (((reg)>>8) & 0xffff)
#define SUBINDEX(reg) ((reg) & 0xff)

static int bus_send_cmd_set(Bus* bus, uint32_t serial, uint32_t reg, uint32_t value) {
	bus->pending++;
	return fprintf(bus->ctl, "0x%x:0x%x[%d] := 0x%x\n", serial, INDEX(reg), SUBINDEX(reg), value);
}

static int bus_send_cmd_get(Bus* bus, uint32_t serial, uint32_t reg) {
	bus->pending++;
	return fprintf(bus->ctl, "0x%x:0x%x[%d]\n", serial, INDEX(reg), SUBINDEX(reg));
}

// time in ms since first call.
static long now_ms() {
	static struct timeval tv_zero = { 0 , 0 };
	if (tv_zero.tv_sec == 0) {
		if (gettimeofday(&tv_zero, NULL) < 0) {
			fprintf(stderr, "No working clock:%s\n", strerror(errno));
			exit(1);
		}
		return 0;
	}

	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0) {
		fprintf(stderr, "No working clock:%s\n", strerror(errno));
		exit(1);
	}

	long ms = tv.tv_usec;
	ms -= tv_zero.tv_usec;
	ms /= 1000;
	return 1000 * (tv.tv_sec - tv_zero.tv_sec) + ms;
}

enum { TIMEOUT_MS = 1000 };

void bus_clocktick(Bus* bus) {
	Device* dev;
	Register* reg;
	long t = now_ms();
	for (dev = bus->devices; dev; dev = dev->next)
		for (reg = dev->registers; reg; reg = reg->next) {
			if (reg->state != PENDING) continue;
			// guard against clock jumps
			if ((t < reg->issued_ms) || (t - reg->issued_ms > TIMEOUT_MS))
				reg->state = INVALID;
		}
}


Device* bus_open_device(Bus* bus, uint32_t serial) {
	Device* dev;
	for (dev = bus->devices; dev; dev = dev->next)
		if (dev->serial == serial)
			return dev;
	dev = malloc(sizeof(*dev));
	dev->serial = serial;
	dev->registers = NULL;
	dev->bus = bus;
	dev->next = bus->devices;
	bus->devices = dev;
	return dev;
}

void bus_close_device(Device* dev) {
	Device** prevp = &(dev->bus->devices);
	while (*prevp) {
		Device* curr = *prevp;
		if(curr == dev) {
			*prevp = curr->next;
			free(curr);
			return;
		}
		prevp = &curr->next;
	}
}


static Register* find_reg(Device* dev, uint32_t regidx) {
	Register* reg;
	for (reg = dev->registers; reg; reg = reg->next)
		if (reg->reg == regidx)
			return reg;

	reg = malloc(sizeof(*reg));
	reg->reg = regidx;
	reg->state = INVALID;
	reg->next = dev->registers;
	dev->registers = reg;
	return reg;
}

int device_get_register(Device* dev, uint32_t regidx, uint32_t* val) {
	Register* reg = find_reg(dev, regidx);

	switch (reg->state) {
	case VALID:
		*val = reg->value;
		return 1;
	case INVALID:
		if (bus_send_cmd_get(dev->bus, dev->serial, regidx) > 0) {
			reg->state = PENDING;
			reg->issued_ms = now_ms();
		}
		// fallthrough
	case PENDING:
		return 0;
	}
	return 0;
}

int device_set_register(Device* dev, uint32_t regidx, uint32_t val) {
	Register* reg = find_reg(dev, regidx);

	if (reg->state == PENDING && reg->value == val)
		return 0;

	if (reg->state == VALID && reg->value == val)
		return 1;

	if (bus_send_cmd_set(dev->bus, dev->serial, regidx, val) > 0) {
		reg->value = val;
		reg->state = PENDING;
		reg->issued_ms = now_ms();
	} else {
		reg->state = INVALID;
	}
	return 0;
}

void device_invalidate_register(Device* dev, uint32_t regidx) {
	Register* reg;
	for (reg = dev->registers; reg; reg = reg->next)
		if (reg->reg == regidx)
			break;
	if (!reg) return;
	reg->state = INVALID;
}
