// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LIB_CONFIG_PROPERTIES_H__
#define LIB_CONFIG_PROPERTIES_H__

#include "io/ipc/key_value_pairs.h"

// Load key/value properties by merging an arary of hard-coded
// defaults with additional entries loaded from a file.
bool LoadProperties(const char *defaults[][2], const char *filename,
                    KeyValuePair *properties);

#endif // LIB_CONFIG_PROPERTIES_H__
