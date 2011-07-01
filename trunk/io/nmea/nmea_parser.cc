// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include <algorithm>
#include <cstdio>
#include <string>
#include <set>

#include "io/nmea/nmea_parser.h"

NmeaParser::NmeaParser()
  : num_bytes_read_(0),
    num_errors_(0),
    num_correct_sentences_(0) {
}

using std::string;
using std::set;

namespace {
// Sets value to the integer value corresponding to hex_digit
// (between '0' and '9' or 'A' and 'F'). If hex_digit is not
// a valid hexadecimal digit, returns false.
bool GetHexDigitValue(char hex_digit, int* value) {
  if (hex_digit >= '0' && hex_digit <= '9') {
    *value = hex_digit - '0';
    return true;
  } else if (hex_digit >= 'A' && hex_digit <= 'F') {
    *value = (hex_digit - 'A') + 10;
    return true;
  } else {
    return false;
  }
}

// Checks that checksum_chars are valid hex characters, populates
// output with their value and returns true on success.
bool ParseChecksum(const string& checksum_chars, int* output) {
  if (checksum_chars.size() != 2) return false;

  int digits[2];
  if (!GetHexDigitValue(checksum_chars[0], &digits[0]) ||
      !GetHexDigitValue(checksum_chars[1], &digits[1])) {
    return false;
  }

  *output = (digits[0] * 0x10) + digits[1];
  return true;
}
}  // anonymous namespace

NmeaParser::Result NmeaParser::Failure(
    NmeaParser::Result error_code,
    int bytes_read,
    NmeaSentence* output) {
  // Assert(error_code != SENTENCE_PARSED)
  output->parts.clear();
  num_bytes_read_ += bytes_read;
  ++num_errors_;
  return error_code;
}

NmeaParser::Result NmeaParser::Success(int bytes_read) {
  num_bytes_read_ += bytes_read;
  ++num_correct_sentences_;
  return SENTENCE_PARSED;
}

NmeaParser::Result NmeaParser::Parse(const string& nmea_line,
                                     NmeaSentence* output) {
  const size_t end_of_line = std::min(nmea_line.size(),
                                      nmea_line.find_first_of("\n\r"));
  // Assert(output != NULL)
  output->parts.clear();
  if (nmea_line.empty()) {
    return Failure(INCORRECT_START_CHAR, 0, output);
  }

  set<char> start_chars;
  start_chars.insert('$');
  start_chars.insert('!');
  if (start_chars.find(nmea_line[0]) == start_chars.end()) {
    return Failure(INCORRECT_START_CHAR, 1, output);
  }

  size_t start = 0;
  for (size_t found = nmea_line.find(',', start);
       found != string::npos;
       found = nmea_line.find(',', start)) {
    // Assert(found != string::npos)
    output->parts.push_back(nmea_line.substr(start, found - start));
    start = found + 1;
  }

  size_t checksum_start = nmea_line.find('*', start);
  if (checksum_start == string::npos ||          // Checksum must be present.
      end_of_line - checksum_start != 3) {  // Two checksum digits and '*'.
    return Failure(CHECKSUM_MISSING,
                   std::min(end_of_line, checksum_start),
                   output);
  }
  output->parts.push_back(nmea_line.substr(start, checksum_start - start));

  if (!ParseChecksum(nmea_line.substr(checksum_start + 1, 2),
                     &output->receivedChecksum)) {
    return Failure(MALFORMED_CHECKSUM, end_of_line, output);
  }

  // TODO(rekwall): calculate the checksum of the sentence here.

  return Success(end_of_line);
}

string NmeaSentence::DebugString() const {
  if (parts.empty()) {
    return "(Invalid/empty sentence)";
  }

  string result = parts[0];
  for (int i = 1; i < parts.size(); ++i) {
    result.append("," + parts[i]);
  }
  char checksum_chars[3];
  snprintf(checksum_chars, 3, "%02X", checksum);
  result.append(checksum_chars, 2);
  result.append("\n");
  return result;
}
