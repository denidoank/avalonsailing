// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
//
#ifndef COMMON_LIMIT_RATE_H_
#define COMMON_LIMIT_RATE_H_

void LimitRate(double in, double max_delta, double* follows);

void LimitRateWrapRad(double in, double max_delta, double* follows);

#endif  // COMMON_LIMIT_RATE_H_
