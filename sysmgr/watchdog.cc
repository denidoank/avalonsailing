// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include <sysmgr/watchdog.h>

#include <fcntl.h>
#include <linux/watchdog.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>


Watchdog::Watchdog(int timeout, bool test_only) : watchdog_fd_(-1),
                                                  timeout_(timeout),
                                                  test_only_(test_only) {
  Init();
};

Watchdog::~Watchdog() {
  if (watchdog_fd_ != -1) {
    close(watchdog_fd_);
  }
};

void Watchdog::Keepalive() {
  if (!Init()) {
    return;
  }
  write(watchdog_fd_, "x", 1); // write anything to watchdog device
};

bool Watchdog::Init() {
  if (test_only_ || watchdog_fd_ != -1) {
    return true;
  }
  // Open watchdog device with linux special option close on exec, since the
  // launcher will need to fork some children.
  // This assumes that kernel watchdog is compiled with CONFIG_WATCHDOG_NOWAYOUT
  // and or supports magic close, which will not automatically disable the
  // watchdog when the device is closed.
  watchdog_fd_ = open("/dev/watchdog", O_RDWR|O_CLOEXEC);
  // If this fails, we hope it's transient and keep trying next time
  if (watchdog_fd_ == -1) {
    fprintf(stderr, "Failure to open watchdog device\n");
    return false;
  }
  // We try to set our intended timeout value, but there is nothing we
  // can do if it fails, other than live with it
  if(ioctl(watchdog_fd_, WDIOC_GETTIMEOUT, &timeout_) != 0) {
    fprintf(stderr, "Failure to set watchdog timout\n");
  }
  if(ioctl(watchdog_fd_, WDIOC_GETTIMEOUT, &timeout_) != 0) {
    fprintf(stderr, "Failure to read current watchdog timeout\n");
  }
  fprintf(stderr, "Watchdog is active with %d timout\n", timeout_);
  return true;
};
