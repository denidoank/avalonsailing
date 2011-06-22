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

  Properties prop(defaults);
  PF_TEST(prop.LoadFromFile("./properties_test.txt"),
          "load properties");


  std::string default_string = "";
  long default_long = 0;
  double default_double = 0.0;
  bool default_bool = false;

  // Get original value from defaults
  PF_TEST(prop.Get("a", default_string) == "a1", "get a='a1'");
  // File value overrides defaults
  PF_TEST(prop.Get("b", default_string) == "2", "get b=2");

  // Parse as long (values from properties_test.txt)
  PF_TEST(prop.Get("c", default_long) == 3, "get c=3"); // overrides defaults
  PF_TEST(prop.Get("e", default_long) == 4, "get e=4"); // from file only

  // Parse different types of boolean representations
  // (values from properties_test.txt)
  PF_TEST(prop.Get("f", default_bool) == false, "get f=false");
  PF_TEST(prop.Get("g", default_bool), "get g=true");
  PF_TEST(prop.Get("h", default_bool), "get h=1");
  PF_TEST(prop.Get("i", default_bool), "get i=t");

  // Parse as double
  PF_TEST(prop.Get("e", default_double) == 4.0, "get e as double");

  // Return hard-coded default if property key is not in map
  // (neither defaults or file)
  PF_TEST(prop.Get("z", default_string) == "",
          "get non-existant string value");
  PF_TEST(prop.Get("z", default_long) == 0, "get non-existant long value");
  PF_TEST(prop.Get("z", default_double) == 0.0,
          "get non-existant double value");
  PF_TEST(prop.Get("z", default_bool) == false,
          "get non-existant boolean value");
}
