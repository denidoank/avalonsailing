// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LIB_UTIL_SERIAL_H__
#define LIB_UTIL_SERIAL_H__

#include <stdio.h>

#include "lib/util/reader.h"

// Wrapper class for a bi-directional serial-port.
//
// Input is handled by non-blocking reader and
// Output is assumed to be using some variant of
// fprintf.
class Serial {
 public:
  Serial(): serial_out_(NULL) {};
  ~Serial();

  // If init returns false, the object may be in an inconsistent state
  // must not be used and should be destroyed.
  bool Init(const char *devname, int speed);

  Reader &In() { return serial_in_; }
  int Printf(const char *fmt, ...) __attribute__((format(printf, 2, 3)));

 private:
  Reader serial_in_;
  FILE *serial_out_;
};

#endif // LIB_UTIL_SERIAL_H__
