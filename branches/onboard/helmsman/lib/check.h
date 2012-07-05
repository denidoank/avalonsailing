// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
//
// Unit test stuff, needs more verbosity
#ifndef COMMON_CHECK_H
#define COMMON_CHECK_H

#include <assert.h>
#include <math.h>  // fabs

#define CHECK(a) assert(a)
#define CHECK_EQ(a, b) assert((a) == (b))
#define CHECK_NE(a, b) assert((a) != (b))
#define CHECK_NOT_NULL(a) (assert((a) != NULL), a)
#define CHECK_FLOAT_EQ(a, b) assert(fabs((a) - (b)) < 1E-5)
#define CHECK_LT(a, b) assert((a) < (b))
#define CHECK_LE(a, b) assert((a) <= (b))
#define CHECK_GE(a, b) assert((a) >= (b))
#define CHECK_GT(a, b) assert((a) > (b))
#define CHECK_IN_INTERVAL(a, x, b) assert((a) <= (x) && (x) <= (b))

#endif  // COMMON_CHECK_H
