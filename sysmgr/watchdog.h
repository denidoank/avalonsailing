// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef AVALON_SYSMGR_WATCHDOG_H_
#define AVALON_SYSMGR_WATCHDOG_H_

/*
 * Controller for the Linux kernel hardware watchdog.
 *
 * Watchdog activates on creation of this class and must
 * be fed periodically by calling the Keepalive method.
 * For testing, the watchdog can be turned into a no-op.
 *
 * For details see:
 * http://www.kernel.org/doc/Documentation/watchdog/watchdog-api.txt
 *
 */
class Watchdog {
 public:

  Watchdog(int timeout, bool test_only);
  ~Watchdog();

  void Keepalive();
  int GetTimeout() { return timeout_; }

 private:
  bool Init();

  int watchdog_fd_;
  int timeout_;
  bool test_only_;
};

#endif // AVALON_SYSMGR_WATCHDOG_H_
