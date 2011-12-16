// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
// December 2011

#ifndef COMMON_PROBE_H_
#define COMMON_PROBE_H_

class Probe {
public:
  Probe();
  void Reset();
  void Measure(double in);
  double Value();
private:
  int samples_;
  double sum_;
};

#endif  // COMMON_PROBE_H_
