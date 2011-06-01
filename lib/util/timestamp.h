// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LIB_UTIL_TIMESTAMP_H__
#define LIB_UTIL_TIMESTAMP_H__

#include <string>
#include <time.h>

// Class to represent a second resolution timestamp
// and its conversion to and from an ISO 8601 formatted
// string (e.g. 2011-05-15T11:53:42).
class Timestamp {
 public:
  Timestamp() { ts_ = time(NULL); };

  // Seconds elapsed between prev timestamp and this instance.
  long SecondsSince(const Timestamp &prev) const {
    return ts_ - prev.ts_;
  }

  // Return ISO 8601 formatted timestamp
  std::string ToIsoString() const {
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%FT%T", localtime(&ts_));
    return std::string(buffer);
  }

  // Parse ISO 8601 formatted timestamp (specific format only)
  // and return true if successful.
  bool FromIsoString(const char *timestr) {
    struct tm localtime;

    char *p = strptime(timestr, "%FT%T", &localtime);
    if (p == NULL) {
      return false;
    }
    ts_ = mktime(&localtime);
    return true;
  }

 private:
  time_t ts_;
};

#endif // LIB_UTIL_TIMESTAMP_H__
