// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/util/token_buffer.h"

#include <string.h>

#include "lib/testing/pass_fail.h"

int main(int argc, char **argv) {

  TokenBuffer buffer;

  strncpy(buffer.buffer, "this  is    a test\n", TB_MAX_BUFFER_SIZE);
  PF_TEST(buffer.Tokenize(), "tokenize");
  PF_TEST(buffer.argc == 4, "contains 4 tokens");
  PF_TEST(strcmp(buffer.argv[3], "test") == 0, "last found token is 'test'");

  strncpy(buffer.buffer, "\n", TB_MAX_BUFFER_SIZE);
  PF_TEST(buffer.Tokenize(), "tokenize - empty line");
  PF_TEST(buffer.argc == 0, "contains no tokens");

  strncpy(buffer.buffer, "", TB_MAX_BUFFER_SIZE);
  PF_TEST(buffer.Tokenize(), "tokenize - empty string");
  PF_TEST(buffer.argc == 0, "contains no tokens");
}
