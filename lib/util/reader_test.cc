// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/util/reader.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "lib/testing/pass_fail.h"

void TestReadLine() {
  int pipefd[2];
  PF_TEST(pipe(pipefd) == 0, "create pipe");

  Reader reader;
  char buffer[200];
  PF_TEST(reader.Init(pipefd[0], true), "initialize reader");

  PF_TEST(reader.ReadLine(buffer, sizeof(buffer), 10) == Reader::READ_TIMEOUT,
          "timeout on empty fifo");
  write(pipefd[1], "abcd", 4);
  PF_TEST(reader.ReadLine(buffer, sizeof(buffer), 10) == Reader::READ_TIMEOUT,
          "timeout on partial line/short read");
  write(pipefd[1], "line!\n", 6);
  PF_TEST(reader.ReadLine(buffer, 3, 10) == Reader::READ_OVERFLOW,
          "output buffer overflow");
  PF_TEST(reader.ReadLine(buffer, sizeof(buffer), 10) == Reader::READ_OK,
          "reading first line");
  PF_TEST(strcmp(buffer, "abcdline!") == 0, "compare output");
  write(pipefd[1], "abcd", 4);
  close(pipefd[1]);
  PF_TEST(reader.ReadLine(buffer, sizeof(buffer), 10) == Reader::READ_OK,
          "read line terminated by EOF");
  PF_TEST(strcmp(buffer, "abcd") == 0, "compare output");
  PF_TEST(reader.ReadLine(buffer, sizeof(buffer), 10) == Reader::READ_EOF,
          "EOF");
}

void TestLineOverflow() {
  int pipefd[2];
  PF_TEST(pipe(pipefd) == 0, "create pipe");

  Reader reader;
  char c;
  char buffer[ READER_MAX_LINE_SIZE + 100];
  PF_TEST(reader.Init(pipefd[0], true), "initialize reader");

  // Buffer overflow test
  for (int i = 0; i <  READER_MAX_LINE_SIZE + 10; i++) {
    write(pipefd[1], "a", 1);
  }
  PF_TEST(reader.ReadLine(buffer, sizeof(buffer), 10) == Reader::READ_OVERFLOW,
          "line-buffer overflow");

  int rcount = 0;
  for (int i = 0; i <  READER_MAX_LINE_SIZE + 10; i++) {
    rcount += reader.ReadChar(&c, 10) == Reader::READ_OK ? 1 : 0;
  }
  PF_TEST(rcount ==  READER_MAX_LINE_SIZE + 10,
          "correct number of characters read");
  PF_TEST(reader.ReadChar(&c, 10) == Reader::READ_TIMEOUT,
          "empty fifo");
}

void TestReadChar() {
  int pipefd[2];
  PF_TEST(pipe(pipefd) == 0, "create pipe");

  Reader reader;
  char c;
  char buffer[10];
  PF_TEST(reader.Init(pipefd[0], true), "initialize reader");

  PF_TEST(reader.ReadChar(&c, 10) == Reader::READ_TIMEOUT,
          "timeout on empty fifo");
  write(pipefd[1], "abcd", 4);
  for (int i = 0; i < 4; i++) {
    PF_TEST(reader.ReadChar(&c, 10) == Reader::READ_OK,
            "read char");
    PF_TEST(c == *("abcd" + i), "read expected char");
  }

  // EOF handling test
  write(pipefd[1], "b", 1);
  close(pipefd[1]);
  PF_TEST(reader.ReadChar(&c, 10) == Reader::READ_OK,
          "read from buffer after EOF");
  PF_TEST(reader.ReadChar(&c, 10) == Reader::READ_EOF,
          "EOF");
}

int main(int argc, char **argv) {
  TestReadLine();
  TestReadChar();
  TestLineOverflow();
  return 0;
}
