#include "io/ipc/producer_consumer.h"

#include <iostream>
#include <cstdio>
#include <string>

#include "lib/testing/pass_fail.h"
#include "lib/util/stopwatch.h"
#include "lib/fm/fm.h"

using std::string;
using std::cout;
using std::endl;

int main(int argc, char** argv) {
  string test_file = "/mnt/tmpfs/foobar/foobar.txt";
  if (argc > 1) {
    test_file = argv[1];
  }

  remove(test_file.c_str());
  Consumer reader(test_file);
  Producer writer(test_file);

  string result;
  PF_TEST(reader.Consume(&result) == false, "No writer yet.");

  PF_TEST(writer.Produce("1st data"), "First write.");
  PF_TEST(writer.Produce("2nd data"), "Second write.");
  PF_TEST(reader.Consume(&result), "Consume succeeds after write.");
  PF_TEST(writer.Produce("3rd data"), "Third write.");
  PF_TEST(result.compare("2nd data") == 0, "Consume data is ok.");

  // Check how long it takes to write to a file.
  StopWatch timer;
  char buf[50];
  const int iterations = 1e3;
  for (int i = 0; i < iterations; ++i) {
    snprintf(buf, 50, "File contents at iteration %d is about 50 bytes", i);
    writer.Produce(buf);
  }
  const long elapsed_per_produce_us = timer.Elapsed() * 1000 / iterations;
  PF_TEST(elapsed_per_produce_us < 100,
          "Produce takes less than 100 microseconds on ramfs");
  cout << "Produce of 50 bytes took " << elapsed_per_produce_us
       << " microseconds." << endl;
}
