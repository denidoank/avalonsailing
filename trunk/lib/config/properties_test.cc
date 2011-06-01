// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/config/properties.h"

#include <string>

#include "lib/testing/pass_fail.h"

int main(int argc, char **argv) {
  static const char *defaults[][2] = {{"a", "a1"},
                               {"b", "b1"},
                               {"c", "c1"},
                               {NULL, NULL}};

  KeyValuePair prop;
  PF_TEST(LoadProperties(defaults, "./properties_test.txt", &prop),
          "load properties");

  std::string result;
  PF_TEST(prop.Get("a", &result), "get a");
  PF_TEST(result == "a1", "a is a1");
  PF_TEST(prop.Get("c", &result), "get c");
  PF_TEST(result == "3", "c is 3");
  PF_TEST(prop.Get("e", &result), "get e");
  PF_TEST(result == "4", "e is 4");
}
