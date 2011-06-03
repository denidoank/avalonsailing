#include "io/ipc/key_value_pairs.h"

#include <string>

#include "lib/testing/pass_fail.h"

using std::string;

void TestGetAddForValidKeysAndValues() {
  KeyValuePair kvp;
  string result;
  PF_TEST(kvp.Get("foo", &result) == false, "No keys");

  PF_TEST(kvp.Add("foo", "bar"), "Adding foo");
  PF_TEST(kvp.Get("foo", &result), "Foo is found.");
  PF_TEST(result == "bar", "Value is bar.");

  PF_TEST(kvp.Get("foo", NULL) == false, "NULL output is handled correctly.");

  PF_TEST(kvp.Add("bla", "4"), "Adding second key");
  PF_TEST(kvp.Add("timestamp", "123"), "Adding timestamp");
  PF_TEST(kvp.ToString() == "foo:bar bla:4 timestamp:123",
          "ToString is reasonable");
}

void TestGetAddForInvalidKeysAndValues() {
  KeyValuePair kvp;
  PF_TEST(kvp.Add("colon:", "shouldnt_work") == false, "Separator in key");
  PF_TEST(kvp.Add("colon", "shouldnt\"work") == false, "quote in value");
  PF_TEST(kvp.Add("colon 2", "notgood") == false, "Field separator in key");
  PF_TEST(kvp.Add("colon\"2", "not good") == false, "quote in key");

  string result;
  PF_TEST(kvp.Get("colon:", &result) == false, "Get with separator in key");
  PF_TEST(kvp.Get("colon", &result) == false, "Get with quote in value");
  PF_TEST(kvp.Get("colon 2", &result) == false, "Get with field sep in key");
  PF_TEST(kvp.Get("colon\"2", &result) == false, "Get with quote in key");
}

void TestFromStringForValidKeysAndValues() {
  const string original_string = "bla:23 second:value third:3.123";
  KeyValuePair from_string(original_string);
  string result;
  PF_TEST(from_string.Get("bla", &result), "First key found.");
  PF_TEST(result == "23", "First value OK");
  PF_TEST(from_string.Get("second", &result), "Second value found.");
  PF_TEST(result == "value", "Second value OK");
  PF_TEST(from_string.Get("third", &result), "Third value found.");
  PF_TEST(result == "3.123", "Third value OK");
  PF_TEST(from_string.Get("timestamp", &result) == false, "No timestamp");

  // Check that ToString (without timestamp returns the original string).
  PF_TEST(from_string.ToString(false) == original_string,
          "Original string remains unchanged");
  // Check that timestamp is added by ToString.
  const string to_string = from_string.ToString();
  const string expected_start = "timestamp:";
  PF_TEST(0 == to_string.compare(0, expected_start.size(), expected_start),
          "Starts with timestamp.");
  const int timestamp_size =
      expected_start.size() +
      16 + // timestamp digits.
      1; // field separator char.
  PF_TEST(to_string.size() == timestamp_size + original_string.size(),
          "Only timestamp was added.");
  PF_TEST(0 == to_string.compare(timestamp_size, original_string.size(),
                                 original_string),
          "Other key-values are still the same.");
}

void TestFromStringForDuplicateKeys() {
  KeyValuePair from_string("bla:23 bla:value third:3.123");
  string result;
  PF_TEST(from_string.Get("bla", &result), "First key found once.");
  PF_TEST(result == "23", "First value OK. Second ignored.");
  PF_TEST(from_string.Get("second", &result) == false, "No second value");
  PF_TEST(from_string.Get("third", &result), "Third value found");
  PF_TEST(result == "3.123", "Third value OK");
  PF_TEST(from_string.ToString(false) == "bla:23 third:3.123",
          "ToString drops second 'bla'");
}

void TestFromStringForInvalidKeysAndValues() {
  KeyValuePair from_string("blah:2:3 bla:value foo bar:baz  foobar:3.123");
  string result;
  PF_TEST(from_string.Get("blah", &result) == false, "First value not valid.");
  PF_TEST(from_string.Get("bla", &result), "Second value is found.");
  PF_TEST(result == "value", "Second value OK");
  PF_TEST(from_string.Get("foo bar", &result) == false, "Third key is bad.");
  PF_TEST(from_string.Get("bar", &result), "Third key is split.");
  PF_TEST(result == "baz", "Third value from split key.");
  PF_TEST(from_string.Get(" foobar", &result) == false, "No space in key.");
  PF_TEST(from_string.Get("foobar", &result), "Last key still there!");
  PF_TEST(result == "3.123", "Last value is there.");
  PF_TEST(from_string.ToString(false) == "bla:value bar:baz foobar:3.123",
          "KeyValuePair only keeps valid key-value pairs.");
}

void TestFromStringForDegenerateKeysAndValues() {
  string result;
  {
    KeyValuePair from_string("a:b : c:d");
    PF_TEST(from_string.Get("a", &result), "First key is in map.");
    PF_TEST(result == "b", "First value is correct.");
    PF_TEST(from_string.Get("c", &result), "Second key is in map.");
    PF_TEST(result == "d", "Second value is correct.");
    PF_TEST(from_string.Get("", &result) == false, "Empty key is not in map.");
    PF_TEST("a:b c:d" == from_string.ToString(false), "Good pairs are in map.");
  }
  {
    KeyValuePair from_string(":a");
    PF_TEST(from_string.Get("", &result) == false, "Empty key is not in map.");
    PF_TEST(from_string.ToString(false) == "", "Map is empty.");
  }
  {
    KeyValuePair from_string("a: b:c");
    PF_TEST(from_string.Get("a", &result) == false, "Empty value not in map.");
    PF_TEST(from_string.Get("b", &result), "Second key is in the map.");
    PF_TEST(from_string.ToString(false) == "b:c", "Map is empty.");
  }
  {
    KeyValuePair from_string("a:b ");
    PF_TEST(from_string.Get("a", &result), "Correct key is in the map.");
    PF_TEST(result == "b", "Value is correct.");
    PF_TEST(from_string.ToString(false) == "a:b", "Map is normalized.");
  }
  {
    KeyValuePair from_string(" a:b");
    PF_TEST(from_string.Get("a", &result), "Correct key is in the map.");
    PF_TEST(result == "b", "Value is correct.");
    PF_TEST(from_string.ToString(false) == "a:b", "Map is normalized.");
  }
  {
    KeyValuePair from_string("a::b");
    PF_TEST(from_string.Get("a", &result) == false, "Two separator removed.");
    PF_TEST(from_string.ToString(false) == "", "Empty map.");
  }
  {
    KeyValuePair from_string("1:2 :3 4:5");
    string result;
    PF_TEST(from_string.Get("", &result) == false, "Empty key not allowed.");
    PF_TEST(from_string.Get("1", &result), "First key is found.");
    PF_TEST(result == "2", "First value is correct.");
    PF_TEST(from_string.Get("4", &result), "Third key is found.");
    PF_TEST(result == "5", "Third value is correct.");
  }
}

void TestGetWithConversion() {
  KeyValuePair kv("a:42.5str b:42 c:1.5 d:184467440737095516150");

  long long_result;
  double double_result;
  PF_TEST(kv.GetLong("a", &long_result) == false, "Parse error for long");
  PF_TEST(kv.GetDouble("a", &double_result) == false, "Parse error for double");
  PF_TEST(kv.GetLong("d", &long_result) == false, "int overflow");

  PF_TEST(kv.GetLong("b", &long_result), "parsing long");
  PF_TEST(long_result == 42, "value check - long");
  PF_TEST(kv.GetDouble("c", &double_result), "parsing double");
  PF_TEST(double_result == 1.5, "value check - double");
  PF_TEST(kv.GetDouble("b", &double_result), "parsing int as double");
}

void TestQuoting() {
  KeyValuePair kv("a:\"this is a test:\" b:\"123\""
                  " c:\"\" d:\"val\"andsome e:\"unbalanced f:123");
  string result;
  long long_result;
  PF_TEST(kv.Get("a", &result), "parse quoted string value");
  PF_TEST(result == "this is a test:", "return proper unquoted value");
  PF_TEST(kv.GetLong("b", &long_result), "parse quoted long");
  PF_TEST(long_result == 123, "valid long result");
  PF_TEST(kv.Get("c", &result) == false, "empty quoted string");
  PF_TEST(kv.Get("d", &result) == false, "no separator after closing quote");
  PF_TEST(kv.Get("e", &result) == false, "unbalanced quotes");
  PF_TEST(kv.Get("f", &result) == false, "element after unbalanced quotes");
  PF_TEST(kv.ToString(false) == "a:\"this is a test:\" b:123",
          "generated quoted string");
}

void TestMerge() {
  KeyValuePair kv1("a:1 b:2 c:3 d:4");
  KeyValuePair kv2("a:x c:y e:z");

  kv1.MergeFrom(kv2);
  PF_TEST(kv1.ToString(false) == "a:x b:2 c:y d:4 e:z", "merged output");
}

int main() {
  TestGetAddForValidKeysAndValues();

  TestGetAddForInvalidKeysAndValues();

  TestFromStringForValidKeysAndValues();

  TestFromStringForDuplicateKeys();

  TestFromStringForInvalidKeysAndValues();

  TestFromStringForDegenerateKeysAndValues();

  TestGetWithConversion();

  TestQuoting();

  TestMerge();
}
