// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// Sign
// for x < 0  : -1
// for x == 0 :  0
// for x > 0  :  1

#ifndef COMMON_SIGN_H
#define COMMON_SIGN_H

double Sign(double x);

double SignNotZero(double x);

#endif  // COMMON_SIGN_H
