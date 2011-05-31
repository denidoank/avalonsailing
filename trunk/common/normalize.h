// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
#ifndef COMMON_NORMALIZE_H_
#define COMMON_NORMALIZE_H_

// Force angle into [0, 360). This is the standard convention for interfaces.
double NormalizeDeg(double alpha_deg);

// Force angle into (-180, 180]
double SymmetricDeg(double alpha_deg);

// Force radians into [0, 2*pi).
double NormalizeRad(double alpha_rad);

// Force angle into (-pi, pi]
double SymmetricRad(double alpha_rad);

#endif  // COMMON_NORMALIZE_H_

