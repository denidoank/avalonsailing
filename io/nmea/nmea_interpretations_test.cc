#include "io/nmea/nmea_interpretations.h"

#include <string>

#include "io/nmea/nmea_parser.h"
#include "lib/testing/pass_fail.h"

using std::string;

// Processes an NMEA sentence for the tests.
static bool ProcessSentence(const string& sentence, NmeaSentence* output) {
  NmeaParser np;
  npInit(&np);

  for (string::const_iterator i = sentence.begin(); i != sentence.end(); ++i) {
    NmeaParsingState state = npProcessByte(&np, output, *i);
    switch (state) {
      case NMEA_PARSER_STILL_PARSING:
        continue;
      case NMEA_PARSER_SENTENCE_PARSED:
        return true;
      default:
        npPrintRawSentenceData(output);
        return false;
    }
  }
  return false;
}

// Checks that NMEA sentence @nmea is correctly parsed and that @interpreter
// then returns @interpretation_succeeds. If @interpretation_succeeds is true,
// also checks that the output string from @interpreter contains
// @expected_result.
static void Check(const string& nmea, NmeaInterpreter interpreter,
                  const string& expected_output,
                  bool interpretation_succeeds = true) {
  NmeaSentence parsed_sentence;
  PF_TEST(ProcessSentence(nmea, &parsed_sentence),
          ("Sentence parsed " + nmea).c_str());

  string result;
  PF_TEST((*interpreter) (parsed_sentence, &result) == interpretation_succeeds,
          ("Sentence interpreted " + nmea).c_str());
  if (interpretation_succeeds) {
    PF_TEST(result.find(expected_output) != string::npos,
            ("Data OK " + result).c_str());
    PF_TEST(result.find("timestamp:") != string::npos,
            ("Has timestamp: " + result).c_str());
  }
}

int main() {
  Check(
      "$WIMWV,214.8,R,0.1,K,A*28", &WIMWVInterpreter,
      "source:wind-sensor angle:214.8 reference:relative speed:0.1 unit:km/h");
  Check("$WIMWV,214.8,R,0.1,K,V*3F", &WIMWVInterpreter,
        "source:wind-sensor invalid:true");
  Check("$WIFOO,214.8,R,0.1,K,A*22", &WIMWVInterpreter, "", false);
  Check("$WIMWV,000,R,0.0,N,V*2A", &WIMWVInterpreter,
        "source:wind-sensor invalid:true");

  Check("$WIXDR,C,21.5,C,2,U,21.2,N,0,U,3.7,V,1,U,3.487,V,2*41",
        &WIXDRInterpreter,
        "source:wind-sensor-voltage temperature:21.5 heating-voltage:21.2 "
        "supply-voltage:3.7 reference-voltage:3.487");
}
