// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LIB_TESTING_PASS_FAIL_H__
#define LIB_TESTING_PASS_FAIL_H__

// Assert like macros for testing.
// Prints PASS or FAIL with a message to stderr depending on
// the test condition being true or false.

#include <stdio.h>

#define PF_TEST(condition, message) do {if (condition) {                \
          fprintf(stderr, "PASS %s %d: %s\n", __FILE__, __LINE__, message); \
        } else { \
          fprintf(stderr, "FAIL %s %d: %s\n", __FILE__, __LINE__, message); \
    } } while(false)


#endif //  LIB_TESTING_PASS_FAIL_H__
