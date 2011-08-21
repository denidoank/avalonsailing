// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman_status.h"
#include <math.h>

#include <stdio.h>
#include <string.h>

HelmsmanStatus:: HelmsmanStatus() {
  Reset();
}

void HelmsmanStatus::Reset() {
  tacks = 0;
  jibes = 0;
  inits = 0;
  // true wind, global frame
  // Both values are NAN initially.
  direction_true_deg = NAN;
  mag_true_m_s = NAN;
}

void HelmsmanStatus::ToProto(HelmsmanStatusProto* sts) const {    
  sts->tacks = tacks;
  sts->jibes = jibes;
  sts->inits = inits;
  sts->direction_true_deg = direction_true_deg;
  sts->mag_true_m_s = mag_true_m_s;
}
