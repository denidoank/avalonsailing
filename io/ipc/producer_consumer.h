// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Atomic Consumer and Producer classes for one-to-many communication between
// a single producer and (potentially) several consumer processes. The
// Producer code guarantees that a message is written atomically (no partial
// writes: the message is entirely written or not at all). The Consumer code
// guarantees that a message is read atomically (no partial reads).

#ifndef IO_IPC_PRODUCER_CONSUMER_H
#define IO_IPC_PRODUCER_CONSUMER_H

#include <string>

using std::string;

// Reads the contents of a file into a string.
class Consumer {
 public:
  // Creates a Consumer for @path. There can be several Consumers
  // for a given path.
  explicit Consumer(const string& path);

  // Reads the content in the file, stores it in @output and returns true on
  // success. Otherwise, returns false and leaves output untouched.
  bool Consume(string* output) const;

 private:
  const string path_;
};

// Atomically sets the contents of a file to a given string.
class Producer {
 public:
  // Creates a Producer for @path. There should be at most one Producer
  // for a given path.
  explicit Producer(const string& path);

  // Replaces the contents of the file by @content and returns true on success.
  // The function is const: the Producer object is not modified by this call,
  // but the file is.
  bool Produce(const string& content) const;

 private:
  const string path_;
};

#endif
