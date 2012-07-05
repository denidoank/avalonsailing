// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "lib/util/strings.h"

#include <string>
#include <vector>

#include "lib/testing/pass_fail.h"

using std::string;
using std::vector;

void TestThatItWorks() {
  vector<string> result;
  PF_TEST(SplitString("foobar,bla", ',', &result), "Comma-separated.");
  PF_TEST(result.size() == 2, "2 elements found.");
  PF_TEST(result[0] == "foobar" && result[1] == "bla", "Split worked.");

  PF_TEST(SplitString("1 2 34 55", ' ', &result), "Space-separated.");
  PF_TEST(result.size() == 4, "4 elements found.");
  PF_TEST(result[0] == "1" &&
          result[1] == "2" &&
          result[2] == "34" &&
          result[3] == "55", "4 numbers are correct.");

  PF_TEST(SplitString("1, 2, 3", ',', &result), "Spaces and separators.");
  PF_TEST(result.size() == 3, "3 elements found.");
  PF_TEST(result[0] == "1" &&
          result[1] == " 2" &&
          result[2] == " 3", "Output not stripped.");
}

void TestCornerCases() {
  PF_TEST(SplitString("foobar,bla", ',', NULL) == false, "NULL output.");

  vector<string> result;
  PF_TEST(SplitString("a,,c", ',', &result), "Empty element.");
  PF_TEST(result.size() == 2, "Empty element is discarded.");
  PF_TEST(result[0] == "a" && result[1] == "c",
          "Empty element is not in output.");

  PF_TEST(SplitString("a,b,", ',', &result), "Separator at end.");
  PF_TEST(result.size() == 2, "Last separator correctly handled.");
  PF_TEST(result[0] == "a" && result[1] == "b",
          "Last separator not in result.");

  PF_TEST(SplitString("", ',', &result), "Empty string.");
  PF_TEST(result.empty(), "Empty result.");

  PF_TEST(SplitString(",,,,", ',', &result), "Only separators.");
  PF_TEST(result.empty(), "Empty result again.");

  PF_TEST(SplitString("Hello World", ',', &result), "No separator.");
  PF_TEST(result.size() == 1 && result[0] == "Hello World",
          "Original string in result.");
}

int main(int argc, char **argv) {
  TestThatItWorks();
  TestCornerCases();
}
