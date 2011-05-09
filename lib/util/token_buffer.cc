// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/util/token_buffer.h"

#include <string.h>

bool TokenBuffer::Tokenize() {
  char *current = buffer;
  for (argc = 0; argc < TB_MAX_TOKENS; argc++) {
    do {
      argv[argc] = strsep(&current, "\n ");
      if (argv[argc] == NULL) {
        return true;
      }
    } while(argv[argc][0] == '\0'); // skip empty tokens
  }
  return false;
}
