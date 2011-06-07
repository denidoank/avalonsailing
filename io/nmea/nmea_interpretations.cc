// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "io/nmea/nmea_interpretations.h"

#include <cstring>
#include <string>

#include "io/ipc/key_value_pairs.h"
#include "io/nmea/nmea_parser.h"

using std::string;

bool WIMWVInterpreter(const NmeaSentence& sentence, string* out) {
  if (out == NULL) return false;
  if (sentence.argc <= 5) return false;
  if (strcmp(sentence.argv[0], "WIMWV") != 0) return false;

  KeyValuePair result;
  result.Add("source", "wind-sensor");
  if (*sentence.argv[5] != 'A') {
    result.Add("invalid", "true");
  } else {
    result.Add("angle", sentence.argv[1]);
    result.Add("reference", (*sentence.argv[2] == 'R' ?
                             "relative" : "true"));
    // TODO(rekwall): explicitly set the unit of the speed here,
    // when unit support is added to KeyValuePair.
    result.Add("speed", sentence.argv[3]);
    switch (*sentence.argv[4]) {
      case 'K': result.Add("unit", "km/h"); break;
      case 'M': result.Add("unit", "m/s"); break;
      case 'N': result.Add("unit", "knots"); break;
      default:  result.Add("unit", "unknown"); break;
    }
  }
  *out = result.ToString();
  return true;
}

bool WIXDRInterpreter(const NmeaSentence& sentence, string* out) {
  if (out == NULL) return false;
  if (sentence.argc != 17) return false;
  if (strcmp(sentence.argv[0], "WIXDR") != 0) return false;

  KeyValuePair result;
  result.Add("source", "wind-sensor-voltage");
  // NMEA sentence contains four blocks of four arguments. The second argument
  // in each block contains the actual data.
  if (*sentence.argv[1] == 'C' &&  // Transducer id 2 type (temperature).
      *sentence.argv[3] == 'C' &&  // Transducer id 2 units (Celsius).
      *sentence.argv[4] == '2') {  // Transducer id for heating temperature.
    result.Add("temperature", sentence.argv[2]);
  }
  if (*sentence.argv[5] == 'U' &&  // Transducer id 0 type (heating voltage).
      *sentence.argv[7] == 'N' &&  // Transducer id 0 units (disabled? Volts?).
      *sentence.argv[8] == '0') {  // Transducer id for heating voltage.
    result.Add("heating-voltage", sentence.argv[6]);
  }
  if (*sentence.argv[9] == 'U' &&  // Transducer id 1 type (supply voltage).
      *sentence.argv[11] == 'V' &&  // Transducer id 1 units (Volts).
      *sentence.argv[12] == '1') {  // Transducer id for supply voltage.
    result.Add("supply-voltage", sentence.argv[10]);
  }
  if (*sentence.argv[13] == 'U' &&  // Transducer id 2 type (reference voltage).
      *sentence.argv[15] == 'V' &&  // Transducer id 2 units (Volts).
      *sentence.argv[16] == '2') {  // Transducer id for 3.5V reference voltage.
    result.Add("reference-voltage", sentence.argv[14]);
  }

  *out = result.ToString();
  return true;
}
