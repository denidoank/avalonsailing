// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Rate limiting on syslog.
//
#ifndef _IO_RUDDERD2_LOG_H
#define _IO_RUDDERD2_LOG_H

#include <syslog.h>

// Log to syslog with per-priority tokenbucket rate limiting
void slog(int priority, const char *message, ...);

// LOG_CRIT the message and exit(1)
void crash(const char* fmt, ...);

// Install with
//	if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
//	if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");
void fault();

#endif // _IO_RUDDERD2_LOG_H
