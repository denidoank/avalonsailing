// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "io/ipc/key_value_pairs.h"

#include <errno.h>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/time.h>
#include <vector>

#include "lib/util/stopwatch.h"

using std::fixed;
using std::map;
using std::setprecision;
using std::string;
using std::vector;

const char KeyValuePair::kFieldSeparator(' ');
const char KeyValuePair::kKeyValueSeparator(':');
const char KeyValuePair::kValueQuoteChar('"');
const std::string KeyValuePair::kTimestampKey("timestamp");

namespace {
// Splits @key_value using kKeyValueSeparator and fills @key and @value with
// the two parts of the string, and returns true on success. Returns false and
// leaves @key and @value untouched on failure.
bool GetKeyValue(const string& key_value, string* key, string* value) {
  if (key == NULL || value == NULL) return false;

  int separator_pos = key_value.find(KeyValuePair::kKeyValueSeparator);
  // Separator should be in the string.
  if (separator_pos == string::npos) return false;
  // There should be at least one character before the separator.
  if (separator_pos == 0) return false;
  // There should be at least one character after the separator.
  if (separator_pos + 1 >= key_value.size()) return false;
  // The separator should not appear twice.
  if (key_value.find(KeyValuePair::kKeyValueSeparator,
                     separator_pos + 1) != string::npos) {
    return false;
  }
  string raw_value = key_value.substr(separator_pos + 1);
  int quote = raw_value.find(KeyValuePair::kValueQuoteChar);
  if (quote == string::npos) {
    *value = raw_value;
  } else {
    // Quote must start the value.
    if (quote != 0) return false;
    // Quoted empty string is not allowed.
    if (raw_value.size() <= 2) return false;
    // Second quote must be at the end of the value.
    if (raw_value.find(KeyValuePair::kValueQuoteChar, 1) != raw_value.size() - 1)
      return false;
    *value = raw_value.substr(1, raw_value.size() - 2);
  }

  *key = key_value.substr(0, separator_pos);
  return true;
}
}  // anonymous namespace

KeyValuePair::KeyValuePair() : key_value_pairs_(), key_order_() {}

KeyValuePair::KeyValuePair(const string& sentence)
    : key_value_pairs_(), key_order_() {
  int previous = 0;
  int found = 0;
  do {
    found = sentence.find(kFieldSeparator, previous);
    if (found == string::npos) {
      found = sentence.size();
    }
    // Look for opening quote character in current sub-string.
    int quote_found = sentence.find(kValueQuoteChar, previous);
    if (quote_found != string::npos && quote_found < found) {
      // Look for closing quote character.
      found = sentence.find(kValueQuoteChar, quote_found + 1);
      if (found == string::npos) {
        found = sentence.size();
      } else {
        // Adance to first separator after closing quote (should be +1).
        found = sentence.find(kFieldSeparator, found);
        if (found == string::npos) {
          found = sentence.size();
    }
      }
    }
    const string key_value = sentence.substr(previous, found - previous);
    previous = found + 1;
    {
      string key, value;
      // Ignore current pair if key or value cannot be extracted.
      if (!GetKeyValue(key_value, &key, &value)) continue;
      Add(key, value);
    }
  } while (found != sentence.size());
}

bool KeyValuePair::Add(const string& key, const string& value) {
  // Check that key and value don't contain forbidden characters.
  if (key.find(kKeyValueSeparator) != string::npos ||
      key.find(kFieldSeparator) != string::npos) {
    return false;
  }
  if (key.find(kValueQuoteChar) != string::npos ||
      value.find(kValueQuoteChar) != string::npos) {
    return false;
  }
  // Check that the key is not already in the map.
  if (!key_value_pairs_.insert(make_pair(key, value)).second) {
    return false;
  }
  key_order_.push_back(key);
  return true;
}

bool KeyValuePair::Get(const string& key, string* value) const {
  if (value == NULL) return false;

  map<string, string>::const_iterator found = key_value_pairs_.find(key);
  if (found == key_value_pairs_.end()) return false;

  *value = found->second;
  return true;
}

bool KeyValuePair::GetLong(const string& key, long* value) const {
  string value_string;
  if (Get(key, &value_string)) {
    char *endptr=NULL;
    errno = 0;
    *value = strtol(value_string.c_str(), &endptr, 0);
    if (endptr[0] != '\0' || errno == ERANGE) {
      return false;
    }
    return true;
  }
  return false;
}

bool KeyValuePair::GetDouble(const string& key, double* value) const {
  string value_string;
  if (Get(key, &value_string)) {
    char *endptr=NULL;
    errno = 0;
    *value = strtod(value_string.c_str(), &endptr);
    if (endptr[0] != '\0' || errno == ERANGE) {
      return false;
    }
    return true;
  }
  return false;
}

namespace {
string GetTimestamp() {
  std::stringstream stream;
  stream << StopWatch::GetTimestampMicros();
  return stream.str();
}

// Appends key:value to result. If result is not empty, also adds a field
// separator before the key-value.
void AppendKeyValue(const string& key, const string& value, string* result) {
  if (result == NULL) return;

  string quoted_value;
  if (value.find(KeyValuePair::kFieldSeparator) != string::npos ||
      value.find(KeyValuePair::kKeyValueSeparator) != string::npos) {
    quoted_value = string(1, KeyValuePair::kValueQuoteChar) + value +
        string(1, KeyValuePair::kValueQuoteChar);
  } else {
    quoted_value = value;
  }

  const string next_pair =
      (result->empty() ? "" : string(1, KeyValuePair::kFieldSeparator)) +
      key + KeyValuePair::kKeyValueSeparator + quoted_value;
  result->append(next_pair);
}
}  // anonymous namespace

string KeyValuePair::ToString(bool add_timestamp_if_missing) const {
  string result;

  // Add a timestamp if not present in the map.
  if (add_timestamp_if_missing &&
      key_value_pairs_.find(kTimestampKey) == key_value_pairs_.end()) {
    AppendKeyValue(kTimestampKey, GetTimestamp(), &result);
  }

  // Print order is defined by vector of keys.
  typedef vector<string>::const_iterator It;
  for (It it = key_order_.begin(); it != key_order_.end(); ++it) {
    const string& key = *it;
    map<string, string>::const_iterator found = key_value_pairs_.find(key);
    if (found == key_value_pairs_.end()) {
      // TODO(rekwall): it's a bug if we're here. Log an error or exit here.
      continue;
    }
    AppendKeyValue(key, found->second, &result);
  }
  return result;
}
