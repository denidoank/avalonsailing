// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Functions for parsing NMEA sentences.
//
// Usage example:
//  FILE *f = stdin;
//  NmeaParser parser;
//  NmeaSentence sentence;
//  npInit(&np);
//
//  for(c = fgetc(f); c != EOF; c = fgetc(f)) {
//    switch (npProcessByte(&parser, &sentence, c)) {
//      case NMEA_PARSER_STILL_PARSING:
//        break;
//      case NMEA_PARSER_DATA_OVERFLOW_ERROR:
//        printf("Too much data in sentence.\n");
//        break;
//      case NMEA_PARSER_CHECKSUM_ERROR:
//        print("Checksum error for current sentence.\n");
//        break;
//      case NMEA_PARSER_SENTENCE_PARSED:
//        // Do something with the sentence, e.g.
//        // npPrintRawSentenceData(&sentence);
//        // or something based on sentence.argv[0].
//        break;
//      default:
//        printf("Unknown status\n");
//        break;
//    }
//  }

#ifndef IO_NMEA_NMEA_PARSER_H
#define IO_NMEA_NMEA_PARSER_H

#include <stdbool.h>

// Maximum number of bytes that the parser accepts per NMEA sentence.
// The standard is probably 82 bytes:
// http://www.visualgps.net/WhitePapers/NMEAParser/NMEAParserDesign.html
// http://www.gpsinformation.org/dale/nmea.htm
#define NMEA_PARSER_MAX_DATA_LEN 160
// Maximum number of arguments per NMEA sentence.
#define NMEA_PARSER_MAX_ARGS 32

// Data for one parsed NMEA sentence. The data is stored as an array of bytes,
// with pointers to the data for the individual arguments. Checksum data is
// stored separately.
struct NmeaSentence {
  // Data that was read.
  char data[NMEA_PARSER_MAX_DATA_LEN];
  // Pointers into data.
  char *argv[NMEA_PARSER_MAX_ARGS];
  // Number of entries in argv.
  int argc;

  // Checksum received in the sentence.
  int receivedChecksum;
  // Checksum calculated from the sentence data.
  int checksum;
};

// Prints the raw data in NmeaSentence.
void npPrintRawSentenceData(NmeaSentence *sentence);



// Markers in the NMEA sentences.
#define CHECKSUM_START_CHAR '*'
#define SENTENCE_START_CHAR '$'
#define ARG_SEPARATOR_CHAR ','

// Current state of the NMEA parser.
enum NmeaParserInternalState {
  NMEA_PARSER_STATE_SEARCH_FOR_START = 0,  // Searching for start of message.
  NMEA_PARSER_STATE_CMD,  // Getting command and arguments.
  NMEA_PARSER_STATE_CHECKSUM_1,  // Getting first checksum character.
  NMEA_PARSER_STATE_CHECKSUM_2  // Getting second checksum character.
};

// Parser state.
struct NmeaParser {
  // Current internal state of the parser.
  NmeaParserInternalState state;

  // Next byte in sentence->data to be written.
  int index;
  // True if sentence has been finalized.
  bool finalized;

  // Total number of bytes read.
  long totBytes;
  // Total number of sentences with errors (data overflow, incorrect checksum).
  long totErr;
  // Total number of correctly parsed sentences.
  long totSentences;
};

// External state of the NMEA sentence parser. Returned when calling
// the parser to read a byte.
enum NmeaParsingState {
  NMEA_PARSER_STILL_PARSING = 0,  // Sentence not finished parsing yet.
  NMEA_PARSER_SENTENCE_PARSED,  // Sentence finished parsing at this byte.
  NMEA_PARSER_DATA_OVERFLOW_ERROR,  // Internal data structures too small for
                                    // current sentence.
  NMEA_PARSER_INCORRECT_CHECKSUM  // Sentence was parsed, but received checksum
                                  // and actual checksum do not match.
};

// Initializes NmeaParser p.
void npInit(NmeaParser *p);

// Processes one byte of data and returns the state of the NMEA parser after
// processing that byte. If NMEA_PARSER_SENTENCE_PARSED is returned, a sentence
// was correctly parsed at this byte. The same NmeaSentence sentence argument
// should be used as long as the parser returns NMEA_PARSER_STILL_PARSING.
NmeaParsingState npProcessByte(NmeaParser *p,
                               NmeaSentence *sentence,
                               char data);

#endif
