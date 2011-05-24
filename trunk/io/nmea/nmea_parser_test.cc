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
  npInit(&np);

  FILE *f = stdin;
  if (argc >= 2) {
    f = fopen(argv[1], "r");
  }

  if (!f) {
    perror(argv[1]);
    return -1;
  }

  for(c = fgetc(f); c != EOF; c = fgetc(f)) {
    switch (npProcessByte(&np, &sentence, c)) {
      case NMEA_PARSER_SENTENCE_PARSED:
        printf("Sentence parsed correctly: ");
        break;

      case NMEA_PARSER_INCORRECT_CHECKSUM:
        printf("Incorrect checksum. Calculated checksum: 0x%X, "
               "checksum received in sentence: 0x%X.\n",
               sentence.checksum, sentence.receivedChecksum);
        break;

      case NMEA_PARSER_STILL_PARSING:
        // Byte was added. Continue to next one.
        continue;

      case NMEA_PARSER_DATA_OVERFLOW_ERROR:
        printf("Data overflow error for sentence.\n");
        continue;

      default:
        printf("Unknown parsing state.\n");
        continue;
    }
    npPrintRawSentenceData(&sentence);
  }

  printf("Summary:\n Bytes: %ld\n Errors: %ld\n Sentences: %ld\n",
      np.totBytes,
      np.totErr,
      np.totSentences);
  return 0;
}
