// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Tests the parsing functions in nmea_parser.h.
//
// Usage example:
//   g++ -o nmea_parser_test nmea_parser_test.cc nmea_parser.cc
//   ./nmea_parser_test < testdata/parser_test.txt > result.txt
//   diff testdata/parser_test_expected.txt result.txt

#include <stdio.h>
#include <string.h>

#include "io/nmea/nmea_parser.h"

int main(int argc, char *argv[]) {
  int c;
  NmeaParser np;
  NmeaSentence sentence;

  FILE *f = stdin;
  if (argc >= 2) {
    f = fopen(argv[1], "r");
  }

  if (!f) {
    perror(argv[1]);
    return -1;
  }

  char buf[200];
  while (fgets(buf, 200, f) != NULL) {
    switch (np.Parse(buf, &sentence)) {
      case NmeaParser::SENTENCE_PARSED:
        printf("Sentence parsed correctly: \n");
        break;

      case NmeaParser::INCORRECT_CHECKSUM:
        printf("Incorrect checksum. Calculated checksum: 0x%02X, "
               "checksum received in sentence: 0x%02X.\n",
               sentence.checksum, sentence.receivedChecksum);
        break;

      case NmeaParser::CHECKSUM_MISSING:
      case NmeaParser::MALFORMED_CHECKSUM:
        printf("Missing/malformed checksum.\n");
        continue;

      case NmeaParser::INCORRECT_START_CHAR:
        printf("Incorrect start character.\n");
        continue;

      default:
        printf("Unknown parsing state.\n");
        continue;
    }
    printf("%s\n", sentence.DebugString().c_str());
  }

  printf("Summary:\n Bytes: %lld\n Errors: %lld\n Correct sentences: %lld\n",
      np.GetNumBytesRead(),
      np.GetNumErrors(),
      np.GetNumCorrectSentences());
  return 0;
}
