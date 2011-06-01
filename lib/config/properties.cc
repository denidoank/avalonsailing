// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/config/properties.h"

#include <stdio.h>
#include <string.h>

#include "lib/fm/log.h"

bool LoadProperties(const char *defaults[][2], const char *filename,
                    KeyValuePair *properties) {
  if (properties == NULL) {
    return false;
  }
  if (defaults != NULL) {
    for (int i = 0; defaults[i][0] != NULL; i++) {
      if (properties->Add(defaults[i][0], defaults[i][1]) == false) {
        FM_LOG_FATAL("invalid default property %s:%s",
                     defaults[i][0], defaults[i][1]);
      }
    }
  }
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
    properties->MergeFrom(segment);
  }
  return true;
}
