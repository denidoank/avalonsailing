// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LIB_CONFIG_PROPERTIES_H__
#define LIB_CONFIG_PROPERTIES_H__

#include <string>

#include "io/ipc/key_value_pairs.h"

class Properties {
 public:
  Properties() { }
  // Initialize with an array of hard-coded values
  Properties(const char *defaults[][2]);

  // Merge the content of a key-value file with the current property map
  // overriding existing values.
  bool LoadFromFile(const char *filename);

  // Return a converted value of the corresponding type from the properties
  // map, otherwise return the provided hard-coded default value.
  std::string Get(const std::string &key,
                  const std::string &default_string) const;
  double Get(const std::string &key, double default_double) const;
  long Get(const std::string &key, long default_long) const;
  bool Get(const std::string &key, bool default_bool) const;

  // Get const references to underlying key-value pairs object.
  const KeyValuePair &GetKeyVals() { return properties_; }

 private:
  KeyValuePair properties_;
};

#endif // LIB_CONFIG_PROPERTIES_H__
