// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)
//
// Unit test stuff, needs more verbosity
#ifndef LIB_TESTING_TESTING_H
#define LIB_TESTING_TESTING_H

#include <assert.h>
#include <iostream>
#include <math.h>  // fabs
#include <stdio.h>
#include <stdlib.h>


#define TEST(class_name, test_case)  void class_name##_##test_case()

#define EXPECT_EQ(a, b) \
  if ((a) != (b)) { \
    std::cout << __FILE__ << ":" <<__LINE__ \
    << "\nTest a == b failed with expected:\n"; \
    std::cout << (a) << "\n"; \
    std::cout << "versus actual:\n"; \
    std::cout << (b) << "\n"; \
    exit(1); \
  } 

#define EXPECT_FLOAT_EQ(a, b) \
  if (fabs((a) - (b)) > 1E-6) { \
    std::cout << __FILE__ << ":" <<__LINE__ \
    << "\nTest a == b (tol:1E-6) failed with expected:\n"; \
    std::cout << (a) << "\n"; \
    std::cout << "versus actual:\n"; \
    std::cout << (b) << "\n"; \
    exit(1); \
  } 

#define EXPECT_LT(a, b) \
  if ((a) >= (b)) { \
    std::cout << __FILE__ << ":" <<__LINE__ \
    << "\nTest a < b failed with a:\n"; \
    std::cout << (a) << "\n"; \
    std::cout << "versus b:\n"; \
    std::cout << (b) << "\n"; \
    exit(1); \
  } 

#define EXPECT_LE(a, b) \
  if ((a) > (b)) { \
    std::cout << __FILE__ << ":" <<__LINE__ \
    << "\nTest a <= b failed with a:\n"; \
    std::cout << (a) << "\n"; \
    std::cout << "versus b:\n"; \
    std::cout << (b) << "\n"; \
    exit(1); \
  } 

#define EXPECT_GE(a, b) \
  if ((a) < (b)) { \
    std::cout << __FILE__ << ":" <<__LINE__ \
    << "\nTest a >= b failed with a:\n"; \
    std::cout << (a) << "\n"; \
    std::cout << "versus b:\n"; \
    std::cout << (b) << "\n"; \
    exit(1); \
  } 

#define EXPECT_GT(a, b) \
  if ((a) <= (b)) { \
    std::cout << __FILE__ << ":" <<__LINE__ \
    << "\nTest a > b failed with a:\n"; \
    std::cout << (a) << "\n"; \
    std::cout << "versus b:\n"; \
    std::cout << (b) << "\n"; \
    exit(1); \
  } 

#endif  // LIB_TESTING_TESTING_H
