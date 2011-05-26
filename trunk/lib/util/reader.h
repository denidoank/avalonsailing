// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LIB_UTIL_READER_H__
#define LIB_UTIL_READER_H__

#define READER_MAX_LINE_SIZE 1024

// A buffered, line-based reader for non-blocking IO with timeout.
class Reader {
 public:

  enum ReadState{
    READ_OK,
    READ_ERROR,
    READ_EOF,
    READ_TIMEOUT,
    READ_OVERFLOW
  };

  // Dummy constructor
  Reader(): read_pos_(0), insert_pos_(0), fd_(-1),
            own_fd_(false), eof_(false) {}

  // Initialize with an already existing, writable file descriptor
  bool Init(int fd, bool own = true);
  // Initialize with the name of a file, which will be opened for reading
  bool Open(const char *filename);

  // Initialize by opening serial port device.
  // baudrate must be one of the constants from termios.h
  bool OpenSerial(const char *filename, int baudrate);

  ~Reader();
  int GetFd() { return fd_; }

  // Try to read a single character with given timeout in ms. If successfull,
  // the character is not removed from the input buffer.
  ReadState PeekChar(char *buffer, long timeout);
  // Read a single character with given timeout in ms.
  ReadState ReadChar(char *buffer, long timeout);
  // Read a full line of up to size characters with given timeout in ms.
  // (Newline is stripped from buffer).
  ReadState ReadLine(char *buffer, long size, long timeout,
                     char separator = '\n');


 private:
  // Conditions for buffer full & empty
  bool isFull() const;
  bool isEmpty() const;

  // Find the first occurence of c (typically '\n')
  // and return the offset position of this character
  // into the unread buffer.
  // Returns -1 if the character is not currently found
  // in the buffer.
  int FindChar(const char c) const;

  ReadState isReadable(long timeout) const;
  // Read as much data into the buffer as we can.
  ReadState RefillBuffer();

  int fd_;
  bool own_fd_; // Do we need to close the fd?
  bool eof_; // has EOF been reached?

  // Simple fixed-size linear buffer
  // Data is valid between read_pos_ and insert_pos_ - 1
  // as index positions into buffer_.
  // If full, insert_pos_ is out of bounds by one.
  char buffer_[READER_MAX_LINE_SIZE];
  int read_pos_;
  int insert_pos_;

};

#endif // LIB_UTIL_READER_H__
