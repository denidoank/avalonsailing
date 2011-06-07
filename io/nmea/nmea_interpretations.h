// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Functions that transform an NMEA sentence into the corresponding
// human-readable key-value strings that are used by other components
// in the system.

#ifndef IO_NMEA_NMEA_INTERPRETATIONS_H
#define IO_NMEA_NMEA_INTERPRETATIONS_H

#include <string>

struct NmeaSentence;

// Function that takes an NmeaSentence and outputs the string representation
// expected by other components of the system. Returns true on success, false if
// @sentence could not be interpreted.
typedef bool (*NmeaInterpreter) (const NmeaSentence& sentence,
                                 std::string* out);

// Windspeed WIMWV sentences.
bool WIMWVInterpreter(const NmeaSentence& sentence, std::string* out);

// Windsensor heating/voltage WIXDR sentences.
bool WIXDRInterpreter(const NmeaSentence& sentence, std::string* out);

#endif
