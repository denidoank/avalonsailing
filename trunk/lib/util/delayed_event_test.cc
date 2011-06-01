// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/util/delayed_event.h"

#include "lib/testing/pass_fail.h"

class TestEvent : public DelayedEvent {
 public:
  int i;
  TestEvent() : i(0) {}
  virtual void Handle() { i++; }
};

int main(int argc, char **argv) {
  TestEvent te;
  PF_TEST(DelayedEvent::GetWaitTimeS(42) == 42,
          "get delay with empty schedule");
  te.Schedule(42);
  PF_TEST(DelayedEvent::GetWaitTimeS(100) <= 42,
          "get delay with scheduled event");
  // No event schould be eligible for execution, assuming
  // the last 2 statements did not take <42s to execute.
  DelayedEvent::Dispatch();
  te.Schedule(0); // Make event immediately eligible for execution.
  DelayedEvent::Dispatch();
  PF_TEST(te.i == 1, "handler executed only once");
}
