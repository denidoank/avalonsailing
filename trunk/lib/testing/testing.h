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
#include <iomanip>  // setprecision
#include <math.h>  // fabs
#include <stdio.h>
#include <stdlib.h>

namespace testing {
// Runs all tests registered with ATEST macro below. Returns zero iff all
// succeeded.
int RunAllTests();

// Implementation detail, clients should ignore.
struct TestRegistrar { TestRegistrar(const char* name, void (*f)()); };
}  // testing

#define ATEST(class_name, test_case) \
void class_name##_##test_case(); \
static testing::TestRegistrar class_name##_##test_case##_REGISTRAR( \
  #class_name"_"#test_case, class_name##_##test_case); \
void class_name##_##test_case()

#define TEST(class_name, test_case)  void class_name##_##test_case()

#define EXPECT_THAT(cond) \
  if (!(cond)) { \
    std::cout << __FILE__ << ":" <<__LINE__ \
    << "\nTest condition failed with actual:\n"; \
    std::cout << (cond) << "\n"; \
    exit(1); \
  }

#define EXPECT_EQ(a, b) \
  do { \
    if ((a) != (b)) { \
      std::cout << __FILE__ << ":" <<__LINE__ \
      << "\nTest a == b failed with expected:\n"; \
      std::cout << #a " which is " << (a) << "\n"; \
      std::cout << "versus actual:\n"; \
      std::cout << #b " which is " << (b) << "\n"; \
      exit(1); \
    } \
  } while(0)

#define EXPECT_TRUE(a) \
  do { \
    if (!(a)) { \
      std::cout << __FILE__ << ":" <<__LINE__ \
                << "\n" #a " was unexpectedly false.\n"; \
      exit(1); \
    } \
  } while(0)

#define EXPECT_FALSE(a) \
  do { \
    if (a) { \
      std::cout << __FILE__ << ":" <<__LINE__ \
                << "\n" #a " was unexpectedly true.\n"; \
      exit(1); \
    } \
  } while(0)

#define EXPECT_FLOAT_EQ(a, b) \
  do { \
    if (fabs((a) - (b)) > 1E-4) { \
      std::cout << __FILE__ << ":" <<__LINE__ \
      << "\nTest a == b (tol:1E-4) failed with expected:\n"; \
      std::cout << #a " which is " << std::setprecision(16) << (a) << "\n"; \
      std::cout << "versus actual:\n"; \
      std::cout << #b " which is " << (b) << "\n"; \
      exit(1); \
    } \
  } while(0)

// Expect equality with a given tolerance.
#define EXPECT_EQ_TOL(a, b, tol) \
  do { \
    if (fabs((a) - (b)) > tol) { \
      std::cout << __FILE__ << ":" <<__LINE__ \
      << "\nTest a == b (tol:" << tol << ") failed with expected:\n"; \
      std::cout << #a " which is " << std::setprecision(16) << (a) << "\n"; \
      std::cout << "versus actual:\n"; \
      std::cout << #b " which is " << std::setprecision(16) << (b) << "\n"; \
      std::cout << "actual-expected = " << std::setprecision(16) << (b-a) << "\n"; \
      exit(1); \
    } \
  } while(0)

#define EXPECT_LT(a, b) \
  do { \
    if ((a) >= (b)) { \
      std::cout << __FILE__ << ":" <<__LINE__ \
      << "\nTest a < b failed with a:\n"; \
      std::cout << #a " which is " << (a) << "\n"; \
      std::cout << "versus b:\n"; \
      std::cout << #b " which is " << (b) << "\n"; \
      exit(1); \
    } \
  } while(0)

#define EXPECT_LE(a, b) \
  do { \
    if ((a) > (b)) { \
      std::cout << __FILE__ << ":" <<__LINE__ \
      << "\nTest a <= b failed with a:\n"; \
      std::cout << #a " which is " << (a) << "\n"; \
      std::cout << "versus b:\n"; \
      std::cout << #b " which is " << (b) << "\n"; \
      exit(1); \
    } \
  } while(0)

#define EXPECT_GE(a, b) \
  do { \
    if ((a) < (b)) { \
      std::cout << __FILE__ << ":" <<__LINE__ \
      << "\nTest a >= b failed with a:\n"; \
      std::cout << #a " which is " << (a) << "\n"; \
      std::cout << "versus b:\n"; \
      std::cout << #b " which is " << (b) << "\n"; \
      exit(1); \
    } \
  } while(0)

#define EXPECT_GT(a, b) \
  do { \
    if ((a) <= (b)) { \
      std::cout << __FILE__ << ":" <<__LINE__ \
      << "\nTest a > b failed with a:\n"; \
      std::cout << #a " which is " << (a) << "\n"; \
      std::cout << "versus b:\n"; \
      std::cout << #b " which is " << (b) << "\n"; \
      exit(1); \
    } \
  } while(0)

#define EXPECT_IN_INTERVAL(a, x, b) \
  do { \
    if (!((a) <= (x) && (x) <= (b))) { \
      std::cout << __FILE__ << ":" <<__LINE__ \
      << "\nTest x in [a,  b] failed with a: " #a "\n"; \
      std::cout << (a) << "\n"; \
      std::cout << "versus b: " #b "\n"; \
      std::cout << (b) << "\n"; \
      std::cout << "with x: " #x"\n"; \
      std::cout << (x) << "\n"; \
      exit(1); \
    } \
  } while(0)

#endif  // LIB_TESTING_TESTING_H
