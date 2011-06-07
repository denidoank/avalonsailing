// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "lib/util/strings.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

bool SplitString(const string& str, const char delimiter,
                 vector<string>* output) {
  if (output == NULL) return false;

  output->clear();

  size_t previous = 0;
  do {
    size_t found = str.find(delimiter, previous);
    if (found == string::npos) found = str.size();
    if (found != previous) {
      output->push_back(str.substr(previous, found - previous));
    }
    previous = found + 1;
  } while (previous < str.size());

  return true;
}
