// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Functions for parsing NMEA sentences. Parsing is line-oriented (NMEA
// sentences always end with a newline).
//
// Usage example:
//  const string nmea_data = "$WIMWV,134,2304";
//  NmeaParser parser;  // Can/should be reused for several sentences.
//  NmeaSentence sentence;
//  switch (parser.Parse(nmea_data, &sentence)) {
//   case NmeaParser::SENTENCE_PARSED:
//     // Do something with the sentence, e.g.
//     // npPrintRawSentenceData(&sentence);
//     // or something based on sentence.argv[0].
//     break;
//   case NmeaParser::CHECKSUM_ERROR:
//     print("Checksum error for current sentence.\n");
//     break;
//   default:
//     printf("Unknown status. Should not happen.\n");
//     break;

#ifndef IO_NMEA_NMEA_PARSER_H
#define IO_NMEA_NMEA_PARSER_H

#include <stdint.h>
#include <string>
#include <vector>

// Data for one parsed NMEA sentence. The data is stored as an array of bytes,
// with pointers to the data for the individual arguments. Checksum data is
// stored separately.
struct NmeaSentence {
  // The different (comma-separated) components of a NMEA sentence.
  std::vector<std::string> parts;
  // Checksum received in the sentence.
  int receivedChecksum;
  // Checksum calculated from the sentence data.
  int checksum;
  // Gives a debug string representation of this sentence.
  std::string DebugString() const;
};

// Markers in the NMEA sentences.
#define CHECKSUM_START_CHAR '*'
#define SENTENCE_START_CHAR '$'
#define ARG_SEPARATOR_CHAR ','

// Parser state.
class NmeaParser {
 public:
  NmeaParser();

  // External state of the NMEA sentence parser. Returned when calling
  // the parser to read a byte.
  enum Result {
    SENTENCE_PARSED,      // Sentence finished parsing at this byte.
    INCORRECT_CHECKSUM,   // Sentence was parsed, but received checksum
                          // and actual checksum did not match.
    CHECKSUM_MISSING,     // Sentence did not have a checksum.
    MALFORMED_CHECKSUM,   // Checksum part of the sentence could not be
                          // parsed correctly.
    INCORRECT_START_CHAR  // First character is not an NMEA start-of-sentence
                          // character.
  };

  // Processes one line of NMEA data (a \n terminated string).
  // If SENTENCE_PARSED is returned, the line was correctly parsed.
  // Any other return value indicates an error.
  Result Parse(const std::string& nmea_line, NmeaSentence* output);

  uint64_t GetNumBytesRead() const { return num_bytes_read_; }
  uint64_t GetNumErrors() const { return num_errors_; }
  uint64_t GetNumCorrectSentences() const { return num_correct_sentences_; }

 private:
  // Total number of bytes read.
  uint64_t num_bytes_read_;
  // Total number of sentences with errors (data overflow, incorrect checksum).
  uint64_t num_errors_;
  // Total number of correctly parsed sentences.
  uint64_t num_correct_sentences_;

  // Clears the NmeaSentence, updates the errors and bytes read counters and
  // returns the passed in value.
  Result Failure(Result error_code,
                 int bytes_read,
                 NmeaSentence* output);
  // Updates the correct sentences counter and returns SENTENCE_PARSED.
  Result Success(int bytes_read);
};

#endif
