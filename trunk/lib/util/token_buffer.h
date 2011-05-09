// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#ifndef LIB_UTIL_TOKEN_BUFFER_H__
#define LIB_UTIL_TOKEN_BUFFER_H__

#define TB_MAX_TOKENS 16
#define TB_MAX_BUFFER_SIZE 256

// Turn a string into a series of token
// represented by argv/argc convention
// plus next entry in argv is NULL.
class TokenBuffer {
 public:
  int argc;
  char *argv[TB_MAX_TOKENS];
  char buffer[TB_MAX_BUFFER_SIZE];

  // Tokenize valid string in buffer using ' ' or '\n' as the
  // separator.
  bool Tokenize();
};

#endif // LIB_UTIL_TOKEN_BUFFER_H__
