// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lockfile.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

int
lockfile(const char* path)
{
	FILE* f = fopen(path, "r");
	if (f) {
		int pid = -1;
		fscanf(f, "%d", &pid);
		fclose(f);
		if (pid > 0 && kill(pid, 0) == 0)  {
			syslog(LOG_NOTICE, "lock (%s) is taken", path);
			return 0;
		}
		syslog(LOG_NOTICE, "lock (%s) is stale, removing", path);
		unlink(path);
	}

	char tmp[1024];
	snprintf(tmp, sizeof tmp, "%s.tmp.XXXXXX", path);
	int fd = mkstemp(tmp);
	if (fd < 0) {
		syslog(LOG_ERR, "could not create lock tmpfile %s: %s", tmp, strerror(errno));
		return 0;
	}
	if (fchmod(fd, 0644) < 0) {
		syslog(LOG_ERR, "could not chmod lock tmpfile %s: %s", tmp, strerror(errno));
		unlink(tmp);
		return 0;		
	}

	f = fdopen(fd, "w");
	if(!f) {
		syslog(LOG_ERR, "error wrapping tmpfile %s: %s", tmp, strerror(errno));
		unlink(tmp);
		return 0;
	}
	fprintf(f, "%10d\n", getpid());
	fflush(f);
	if (fchmod(fd, 0644) < 0) {
		syslog(LOG_ERR, "could not chmod lock tmpfile %s: %s", tmp, strerror(errno));
		unlink(tmp);
		return 0;		
	}
	fclose(f);

	if (link(tmp, path) < 0) {
		if (errno == EEXIST) 
			syslog(LOG_NOTICE, "someone beat us to create %s", path);
		else 
			syslog(LOG_NOTICE, "could not create %s: %s", path, strerror(errno));
		unlink(tmp);
		return 0;
	}
	syslog(LOG_INFO, "got lock %s", path);
	unlink(tmp);
	return 1;
}


// if the lockfile exists and contains our own pid, remove it.
// returns false if someone else stole the lock.
int
unlockfile(const char* path)
{
	FILE* f = fopen(path, "r");
	if (!f) {
		syslog(LOG_NOTICE, "no such lock: %s", path);
		return 0;
	}

	int pid = -1;
	fscanf(f, "%d", &pid);
	fclose(f);

	if (pid != getpid()) {
		syslog(LOG_NOTICE, "lock %s stolen by pid %d", path, pid);
		return 0;
	}

	syslog(LOG_INFO, "released lock %s", path);	unlink(path);
	return 1;
}
