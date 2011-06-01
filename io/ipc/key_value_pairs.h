// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Code for printing and parsing key-value pairs (with unique keys). The order
// of keys (defined by the sequence of calls to Add() or from the input string)
// is preserved. The key strings can contain any character, except
// (1) the pair-separator character ' ' (KeyValuePair::kFieldSeparator) and
// (2) the key-value separator ':' (KeyValuePair::kKeyValueSeparator)
// (3) the quote characer '"' (KeyValuePair::kValueQuoteChar).
// Value strings may contain separator characters, but then must be wrapped in
// a single set of quote characters - quotes are otherwise not allowed in the
// value string.
//
// This class is used for the communication between the sensors and the other
// parts of the system.

#ifndef IO_IPC_KEY_VALUE_PAIRS_H
#define IO_IPC_KEY_VALUE_PAIRS_H

#include <map>
#include <string>
#include <vector>

class KeyValuePair {
 public:
  // Separator between key-value pairs (' ' in "key1:value1 key2:value2").
  static const char kFieldSeparator;
  // Separator between keys and values (':' in "key1:value1 key2:value2").
  static const char kKeyValueSeparator;
  // Quote character to group strings which contain separators
  static const char kValueQuoteChar;
  // Timestamp key that is always printed, even if not explicitly Add()ed.
  static const std::string kTimestampKey;

  // Creates an empty KeyValuePair that can be populated using Add().
  KeyValuePair();

  // Creates a KeyValuePair from @sentence. @sentence typically comes from a
  // previous call to ToString on another KeyValuePair object.
  explicit KeyValuePair(const std::string& sentence);

  // Returns true if @key, @value were added (@key not already present in the
  // map).
  bool Add(const std::string& key, const std::string& value);

  // Merge values from another KeyValuePair.
  // Replace existing values with new ones and append new ones to key order.
  void MergeFrom(const KeyValuePair &source);

  // Returns true and populates @value if @key is in the map.
  bool Get(const std::string& key, std::string* value) const;
  bool GetLong(const std::string& key, long* value) const;
  bool GetDouble(const std::string& key, double* value) const;

  // Returns a string representation of the key-value pairs. A timestamp
  // value (current time) is added at the beginning of the returned string, if
  // add_timestamp_if_missing is true and if the kTimestampKey was not added
  // previously with Add.
  std::string ToString(bool add_timestamp_if_missing) const;
  std::string ToString() const { return ToString(true); }

 private:
  // Key-value map.
  std::map<std::string, std::string> key_value_pairs_;
  // Vector of keys for the order.
  std::vector<std::string> key_order_;
};

#endif
