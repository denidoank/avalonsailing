// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, August 2011
#ifndef HELMSMAN_HELMSMAN_STATUS_H
#define HELMSMAN_HELMSMAN_STATUS_H

#include "proto/helmsman_status.h"

struct HelmsmanStatus {
  HelmsmanStatus();
  void Reset();
  void ToProto(HelmsmanStatusProto* sts) const;

  int tacks;
  int jibes;
  int inits;
  // true wind, global frame
  // Both values are NAN initially.
  double direction_true_deg;
  double mag_true_m_s;
};

#endif  // HELMSMAN_HELMSMAN_STATUS_H
