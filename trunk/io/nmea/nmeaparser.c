// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "nmeaparser.h"

void npInit(NmeaParser *p) {
  assert(p != NULL);
  memset(p, 0, sizeof(*p));
  p->state = NMEA_PARSER_STATE_SEARCH_FOR_START;
}


// Resets an NmeaSentence.
static void resetSentence(NmeaParser *parser, NmeaSentence *sentence) {
  // Reset parser state.
  parser->index = 0;  // Reset index.
  parser->finalized = false;

  // Reset sentence state.
  memset(sentence, 0, sizeof(*sentence));
  sentence->checksum = 0;
  sentence->receivedChecksum = 0;
  sentence->argc = 1;
  sentence->argv[0] = sentence->data;
}


// Adds a byte of data to parser->sentence. Returns true if there
// is no data overflow (and the sentence has not been finalized before).
static bool addDataToSentence(char data, bool updateChecksum,
                              NmeaParser *parser, NmeaSentence *sentence) {
  if (parser->finalized) {
    return false;
  }
  assert(parser->index < NMEA_PARSER_MAX_DATA_LEN);
  sentence->data[parser->index++] = data;
  if (updateChecksum) {
    sentence->checksum ^= data;
  }
  // Check for overflow, knowing that at least one more char (e.g. final
  // '\0') still needs to be added to this sentence.
  return parser->index < NMEA_PARSER_MAX_DATA_LEN;
}

static void finalizeSentence(NmeaParser *parser, NmeaSentence *sentence) {
  assert(parser->index < NMEA_PARSER_MAX_DATA_LEN);
  assert(!parser->finalized);

  sentence->data[parser->index] = '\0';
  parser->finalized = true;
}

// Starts a new argv entry in parser->sentence. Return true if there was no
// overflow in the number of arguments or amount of data.
static bool startNewArgument(char separator,
                             NmeaParser *parser,
                             NmeaSentence *sentence) {
  // Check that we haven't reached the max number of args.
  if  (sentence->argc >= NMEA_PARSER_MAX_ARGS) {
    return false;
  }
  // Finalize previous argument.
  if (!addDataToSentence('\0', false, parser, sentence)) {
    return false;
  }
  // Update checksum explicitly. Separator is not part of the data.
  sentence->checksum ^= separator;

  // Create a pointer to the new argument.
  assert(sentence->argc < NMEA_PARSER_MAX_ARGS);
  sentence->argv[sentence->argc++] = sentence->data + parser->index;
  return true;
}

// Returns the integer value corresponding to hex_digit (between '0' and '9'
// or 'A' and 'F'). If hex_digit is not a valid hexadecimal digit, returns -1.
static int getHexDigitValue(char hex_digit) {
  if (hex_digit >= '0' && hex_digit <= '9') {
    return hex_digit - '0';
  } else if (hex_digit >= 'A' && hex_digit <= 'F') {
    return (hex_digit - 'A') + 10;
  } else {
    return -1;
  }
}

// Adds the parsed value of hex_digit, shifted left by shift to
// sentence->receivedChecksum.
static bool addToCheckSum(char hex_digit, int shift, NmeaSentence *sentence) {
  int value = getHexDigitValue(hex_digit);
  if (value < 0) {
    return false;
  }
  sentence->receivedChecksum |= (value << shift);
  return true;
}

NmeaParsingState npProcessByte(NmeaParser *p,
                               NmeaSentence *sentence,
                               char data) {
  NmeaParsingState result = NMEA_PARSER_STILL_PARSING;
  p->totBytes++;

  switch(p->state) {
    // Search for start of message '$'.
    case NMEA_PARSER_STATE_SEARCH_FOR_START:
      if(data == SENTENCE_START_CHAR) {
        resetSentence(p, sentence);
        p->state = NMEA_PARSER_STATE_CMD;
      }
      // Else: skip data.
      break;

    // Retrieve command (NMEA Address).
    case NMEA_PARSER_STATE_CMD:
      if (data == ARG_SEPARATOR_CHAR) {
        if(startNewArgument(data, p, sentence)) {
          p->state = NMEA_PARSER_STATE_CMD;
        } else {
          p->state = NMEA_PARSER_STATE_SEARCH_FOR_START;
          p->totErr++;
          result = NMEA_PARSER_DATA_OVERFLOW_ERROR;
        }
      } else if (data == CHECKSUM_START_CHAR) {
        // Start of checksum, end of data.
        finalizeSentence(p, sentence);
        p->state = NMEA_PARSER_STATE_CHECKSUM_1;
      } else if (data == '\r' || data == '\n') {
        // Sentence without checksum.
        finalizeSentence(p, sentence);
        p->totSentences++;
        p->state = NMEA_PARSER_STATE_SEARCH_FOR_START;
        result = NMEA_PARSER_SENTENCE_PARSED;
      } else {
        // Regular data byte.
        if(!addDataToSentence(data, true, p, sentence)) {
          p->state = NMEA_PARSER_STATE_SEARCH_FOR_START;
          p->totErr++;
          result = NMEA_PARSER_DATA_OVERFLOW_ERROR;
        }
      }
      break;

    case NMEA_PARSER_STATE_CHECKSUM_1:
      // High order checksum byte. Shift left by 4.
      if (addToCheckSum(data, 4, sentence)) {
        p->state = NMEA_PARSER_STATE_CHECKSUM_2;
      } else {
        // Invalid checksum digit.
        p->state = NMEA_PARSER_STATE_SEARCH_FOR_START;
        p->totErr++;
        result = NMEA_PARSER_INCORRECT_CHECKSUM;
      }
      break;

    case NMEA_PARSER_STATE_CHECKSUM_2:
      // Low order checksum byte. No shift left.
      if (addToCheckSum(data, 0, sentence) &&
          sentence->checksum == sentence->receivedChecksum) {
        p->totSentences++;
        result = NMEA_PARSER_SENTENCE_PARSED;
      } else {
        // Invalid checksum digit or non-matching checksum.
        p->totErr++;
        result = NMEA_PARSER_INCORRECT_CHECKSUM;
      }
      p->state = NMEA_PARSER_STATE_SEARCH_FOR_START;
      break;

    default:
      // Unknown state. This assertion will fail.
      assert(p->state == NMEA_PARSER_STATE_SEARCH_FOR_START);
  }
  return result;
}

void npPrintRawSentenceData(NmeaSentence *sentence) {
  int i;
  printf("$%s", sentence->argv[0]);
  for (i = 1; i < sentence->argc; ++i) {
    printf(",%s", sentence->argv[i]);
  }
  printf("*%X\n", sentence->checksum);
}
