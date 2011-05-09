// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "lib/util/serial.h"

#include <stdarg.h>

Serial::~Serial() {
  if (serial_out_ != NULL) {
    fclose(serial_out_);
  }
}

bool Serial::Init(const char *devname, int speed) {
  if (!serial_in_.OpenSerial(devname, speed)) {
    return false;
  }
  serial_out_ = fdopen(serial_in_.GetFd(), "w");
  if (serial_out_ == NULL) {
    return false;
  }
  // Unbuffered IO
  if (setvbuf(serial_out_, NULL, _IONBF, 0) != 0) {
    return false;
  }
  return true;
}

int Serial::Printf(const char *fmt, ...) {
  va_list arg_ptr;
  va_start(arg_ptr, fmt);
  int rc = vfprintf(serial_out_, fmt, arg_ptr);
  va_end(arg_ptr);
  return rc;
}
