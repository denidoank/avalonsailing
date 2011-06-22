// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/config/properties.h"

#include <stdio.h>
#include <string.h>

#include "lib/fm/log.h"

Properties::Properties(const char *defaults[][2]) {
  if (defaults != NULL) {
    for (int i = 0; defaults[i][0] != NULL; i++) {
      if (properties_.Add(defaults[i][0], defaults[i][1]) == false) {
        FM_LOG_FATAL("invalid default property %s:%s",
                     defaults[i][0], defaults[i][1]);
      }
    }
  }
}

bool Properties::LoadFromFile(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    FM_LOG_PERROR("could not open properties file");
    return false;
  }
  char buffer[256];
  while (true) {
    if(fgets(buffer, sizeof(buffer), fp) == NULL) {
      break;
    }
    int len = strlen(buffer);
    // Strip newlines.
    if (buffer[len - 1] == '\n') {
      buffer[len - 1] = '\0';
    }
    // Comments and empty lines.
    if (buffer[0] == '#' || strlen(buffer) == 0) {
      continue;
    }
    KeyValuePair segment(buffer);
    properties_.MergeFrom(segment);
  }
  return true;
}

std::string Properties::Get(const std::string &key,
                            const std::string &default_string) const {
  std::string result;
  if (properties_.Get(key, &result)) {
      return result;
  } else {
    return default_string;
  }
}

double Properties::Get(const std::string &key, double default_double) const {
  double result;
  if (properties_.GetDouble(key, &result)) {
    return result;
  } else {
    return default_double;
  }
}

long Properties::Get(const std::string &key, long default_long) const {
  long result;
  if (properties_.GetLong(key, &result)) {
    return result;
  } else {
    return default_long;
  }
}

bool Properties::Get(const std::string &key, bool default_bool) const {
  std::string result;
  if (properties_.Get(key, &result)) {
    if (result == "true" || result == "t" || result == "1") {
      return true;
    } else {
      return false;
    }
  } else {
    return default_bool;
  }
}
