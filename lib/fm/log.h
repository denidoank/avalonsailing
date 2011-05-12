// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LIB_FM_LOG_H__
#define LIB_FM_LOG_H__

#include <string.h>
#include <errno.h>

#include "lib/fm/constants.h"
//
// printf style logging macros by severity level.
//
// (should compile as C but needs to link with C++)
//
#define FM_LOG_FATAL(...) fm_log(FM_LEVEL_FATAL, __FILE__, __LINE__,\
                                   __VA_ARGS__)
#define FM_LOG_ERROR(...) fm_log(FM_LEVEL_ERROR, __FILE__, __LINE__,\
                                   __VA_ARGS__)
#define FM_LOG_WARN(...) fm_log(FM_LEVEL_WARNING, __FILE__, __LINE__,\
                                  __VA_ARGS__)
#define FM_LOG_INFO(...) fm_log(FM_LEVEL_INFO, __FILE__, __LINE__,\
                                  __VA_ARGS__)
#define FM_LOG_DEBUG(...) fm_log(FM_LEVEL_DEBUG, __FILE__, __LINE__,\
                                   __VA_ARGS__)
// Equivalent to perror() libc function
#define FM_LOG_PERROR(msg) fm_log(FM_LEVEL_ERROR, __FILE__, __LINE__,\
                            "%s: %s", msg, strerror(errno))

#define FM_ASSERT(cond) do { if (!(cond)) { FM_LOG_FATAL(\
                          "assertion failed: '%s'", #cond); } } while(false)

// Declaration of underlying logging function.
// Probably no need to use it directly
// (Tell gcc to use printf style convention for argument error
// checking on varargs).
void fm_log(enum FM_LOG_LEVELS level, const char *file, int line,
         const char *fmt, ...)  __attribute__((format(printf, 4, 5)));
#endif // LIB_FM_LOG_H__
