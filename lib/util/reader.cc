// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/util/reader.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/uio.h>
#include <termios.h>
#include <unistd.h>

#include "lib/util/stopwatch.h"

namespace {
// Returns the baudrate that corresponds to speed (which can either be a
// baudrate enum or an int). Returns 0 if the speed is not supported.
speed_t GetBaudrateEnum(int speed) {
  // Return speed if it's already a supported baudrate enum.
  switch (speed) {
    case B4800:
    case B9600:
    case B115200:
      return speed;
  }
  // Otherwise, assume its a number of bits per second and convert to enum.
  switch (speed) {
    case 4800:
      return B4800;
    case 9600:
      return B9600;
    case 115200:
      return B115200;
  }
  // Unsupported speed.
  return 0;
}
}  // anonymous namespace

bool Reader::Init(int fd, bool own) {
  fd_ = fd;
  own_fd_ = own;
  read_pos_ = insert_pos_ = 0;
  eof_ = false;

  // Make fd non-blocking
  int flags;
  if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
    return false;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    return false;
  }
  return true;
}

bool Reader::Open(const char *filename) {
  int fd = open(filename, O_RDWR | FD_CLOEXEC);
  if (fd == -1) {
    return false;
  }
  return Init(fd);
}

bool Reader::OpenSerial(const char *filename, int baudrate) {
  int fd = open(filename, O_RDWR | O_NOCTTY);
  if (fd == -1) {
    return false;
  }

  termios serial_params;
  tcgetattr(fd,&serial_params);

  serial_params.c_cflag = CS8 | CLOCAL | CREAD;
  serial_params.c_iflag = 0;
  serial_params.c_oflag = 0;

  const speed_t baudrate_enum = GetBaudrateEnum(baudrate);
  cfsetispeed(&serial_params, baudrate_enum);
  cfsetospeed(&serial_params, baudrate_enum);

  // input mode to non-canonical (raw?)
  serial_params.c_lflag = 0;

  if (tcsetattr(fd, TCSANOW, &serial_params) == -1) {
    return false;
  }

  return Init(fd);
}

Reader::~Reader() {
  // Close fd only if we own it.
  if (own_fd_ && fd_ != -1) {
    close(fd_);
  }
}

Reader::ReadState Reader::PeekChar(char *buffer, long timeout) {
  ReadState state;
  if (isEmpty()) {
    state = isReadable(timeout);
    if (state != READ_OK) {
      return state;
    }
    state = RefillBuffer();
    if (state != READ_OK) {
      return state;
    }
  }
  if (!isEmpty()) {
    buffer[0] = buffer_[read_pos_];
    return READ_OK;
  }
  return READ_ERROR;
}

Reader::ReadState Reader::ReadChar(char *buffer, long timeout) {
  ReadState state = PeekChar(buffer, timeout);
  if (state == READ_OK)
    read_pos_++;
  return state;
}

Reader::ReadState Reader::ReadLine(char *buffer, long size, long timeout,
                                   char separator) {
  StopWatch timer;
  while (true) {
    int offset;
    if (!isEmpty() && (offset = FindChar(separator)) != -1) {
      if (offset >= size) {
        return READ_OVERFLOW;
      }
      // copy line without trailing separator
      memcpy(buffer, buffer_ + read_pos_, offset);
      buffer[offset] = '\0'; // terminate string (replacing separator)
      if (eof_) {
        read_pos_ = insert_pos_ = 0;
      } else {
        read_pos_ += (offset + 1);
      }
      return READ_OK;
    }
    ReadState status = RefillBuffer();
    switch (status) {
      case READ_ERROR:
        return READ_ERROR;
      case READ_OVERFLOW:
        return READ_OVERFLOW;
      case READ_EOF:
        // Only return EOF, if buffer is flushed
        if (isEmpty()) {
          return READ_EOF;
        }
        break;
      case READ_TIMEOUT: // Means no data could be read without blocking
        {
          int time_left = timeout - timer.Elapsed();
          if (time_left <= 0) {
            return READ_TIMEOUT;
          }
          ReadState wait_state = isReadable(time_left);
          if (wait_state != READ_OK) {
            return wait_state;
          }
        }
        break;
      case READ_OK:
        break;
    }
  }
}

bool Reader::isFull() const {
  return (insert_pos_ - read_pos_) == sizeof(buffer_);
}

bool Reader::isEmpty() const {
  return read_pos_ == insert_pos_;
}

int Reader::FindChar(const char c) const {
  for (int i = 0; (read_pos_ + i) < insert_pos_; i++ ) {
    if (buffer_[read_pos_ + i] == c) {
      return i;
    }
  }
  if (eof_) {
    return insert_pos_ - read_pos_;
  }
  return -1;
}

Reader::ReadState Reader::isReadable(long timeout) const {
  pollfd poll_request;

  poll_request.fd = fd_;
  poll_request.events = POLLIN;

  int ret = poll(&poll_request, 1, timeout);
  // Since we have only one fd, we know from the return code
  // if we can read or not or there was an error.
  switch (ret) {
    case 0:
      return READ_TIMEOUT;
    case 1:
      return READ_OK;
    default:
      // Typically ret == -1 and errno set to a particular error code
      // This would also happen if reader is not initialized and
      // (fd == -1)
      return READ_ERROR;
  }
}


Reader::ReadState Reader::RefillBuffer() {
  // Before reading, compact the buffer again
  if (isEmpty()) {
    read_pos_ = 0;
    insert_pos_ = 0;
  } else if (isFull()) {
    return READ_OVERFLOW;
  } else if (read_pos_ > 0) {
    int size = insert_pos_ - read_pos_;
    memmove(buffer_, buffer_ + read_pos_, size);
    read_pos_ = 0;
    insert_pos_ = size;
  }

  // At this point buffer is left justified and has some
  // free space for reading data.
  ssize_t read_size = read(fd_, buffer_ + insert_pos_,
                           sizeof(buffer_) - insert_pos_);

  if (read_size == -1) {
    // No data is a special kind of error(s)
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return READ_TIMEOUT;
    } else {
      return READ_ERROR;
    }
  } else if (read_size == 0) {
    eof_ = true;
    return READ_EOF;
  } else {
    insert_pos_ += read_size;
    return READ_OK;
  }
}
