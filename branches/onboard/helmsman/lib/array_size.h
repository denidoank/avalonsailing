// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
#ifndef COMMON_ARRAY_SIZE_H_
#define COMMON_ARRAY_SIZE_H_
// int is more practical for loops.
#define ARRAY_SIZE(x) \
  (int(sizeof(x) / sizeof((x)[0])))
#endif  // COMMON_ARRAY_SIZE_H_
