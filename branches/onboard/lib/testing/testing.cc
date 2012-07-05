// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include <vector>

#include "testing.h"

namespace testing {

typedef std::pair<const char*, void (*)()> Test;
static std::vector<Test>* all_tests = 0;

int RunAllTests() {
  if (!all_tests) {
    std::cerr << "No tests found! Use ATEST macro to register them\n";
    return 1;
  }

  for (size_t i = 0; i < all_tests->size(); ++i) {
    std::cerr << "===== RUNNING " << all_tests->at(i).first << "\n";
    all_tests->at(i).second();
  }
  std::cerr << "PASS\n";
  return 0;
}

TestRegistrar::TestRegistrar(const char* name, void (*f)()) {
  if (!all_tests) {
    all_tests = new std::vector<Test>();
  }
  all_tests->push_back(std::make_pair(name, f));
}
}  // testing
