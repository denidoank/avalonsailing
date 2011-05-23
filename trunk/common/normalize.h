// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
#ifndef COMMON_NORMALIZE_H_
#define COMMON_NORMALIZE_H_

// Force angle into [0, 360). This is the standard convention for interfaces.
double NormalizeAngle(double alpha);

// Force angle into (-180, 180]
double SymmetricAngle(double alpha);

// Force radians into [0, 2*pi).
double NormalizeRadians(double alpha);

// Force angle into (-pi, pi]
double SymmetricRadians(double alpha);

#endif  // COMMON_NORMALIZE_H_

