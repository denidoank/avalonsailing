// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: Steffen Grundmann
//
#ifndef COMMON_LIMIT_RATE_H_
#define COMMON_LIMIT_RATE_H_

// follows does follow in with max_delta change per call.
void LimitRate(double in, double max_delta, double* follows);

// The same for angles in radians with wrap around.
void LimitRateWrapRad(double in, double max_delta, double* follows);

#endif  // COMMON_LIMIT_RATE_H_
