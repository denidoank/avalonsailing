// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LOG_UTIL_STRINGS_H
#define LOG_UTIL_STRINGS_H

#include <string>
#include <vector>

// Clears @output, splits @str into the components delimited by @delimiter,
// adds them to @output and returns true. If consecutive delimiter characters
// appear in @str, they are skipped and no empty components are added to
// @output. Returns false if @output is NULL.
bool SplitString(const std::string& str, const char delimiter,
                 std::vector<std::string>* output);

#endif
