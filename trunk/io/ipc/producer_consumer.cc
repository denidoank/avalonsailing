// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include "io/ipc/producer_consumer.h"

#include <cerrno>
#include <cstdio>  // For rename(...).
#include <cstring> // For strerror(...).
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "lib/fm/log.h"

using std::string;

Consumer::Consumer(const string& path) : path_(path) {}

bool Consumer::Consume(string* output) const {
  if (output == NULL) return false;

  int input_fd = open(path_.c_str(), O_RDONLY);
  if (input_fd == -1) {
    FM_LOG_ERROR("Could not open input file %s: %s\n",
                 path_.c_str(),
                 strerror(errno));
    return false;
  }

  struct stat filestats;
  if (fstat(input_fd, &filestats) == -1) {
    FM_LOG_ERROR("Could not stat file %s: %s\n",
                 path_.c_str(),
                 strerror(errno));
    close(input_fd);
    return false;
  }

  const long file_size = filestats.st_size;

  char* file_contents = new char[file_size];
  FM_ASSERT(file_contents != NULL);

  int pos = 0;
  do {
    int read_count = read(input_fd, file_contents + pos, file_size - pos);
    if (read_count < 0) {
      FM_LOG_ERROR("Read failed on %s: %s\n",
                   path_.c_str(),
                   strerror(errno));
      close(input_fd);
      delete file_contents;
      return false;
    }
    pos += read_count;
  } while (pos < file_size);

  output->assign(file_contents, file_size);
  close(input_fd);
  delete file_contents;
  return true;
}

Producer::Producer(const string& path) : path_(path) {}

bool Producer::Produce(const string& content) const {
  const string tmp_file = path_ + ".tmp";
  // This process is the only one modifying the file, so mode: u=rw,g=r,o=r.
  int output_fd = open(tmp_file.c_str(), O_WRONLY | O_CREAT,
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (output_fd < 0) {
    FM_LOG_ERROR("Could not open output file %s: %s\n",
                 tmp_file.c_str(),
                 strerror(errno));
    return false;
  }

  const int content_size = content.size();
  int pos = 0;
  do {
    int write_count =
        write(output_fd, content.c_str() + pos, content_size - pos);
    if (write_count < 0) {
      FM_LOG_PERROR("Could not write content to temp file");
      close(output_fd);
      return false;
    }
    pos += write_count;
  } while (pos < content_size);

  if (close(output_fd) != 0) {
    FM_LOG_ERROR("Could not close file %s: %s\n",
                 tmp_file.c_str(),
                 strerror(errno));
    return false;
  }

  if (rename(tmp_file.c_str(), path_.c_str()) != 0) {
    FM_LOG_ERROR("Could not rename %s to %s: %s\n",
                 tmp_file.c_str(),
                 path_.c_str(),
                 strerror(errno));
    return false;
  }

  return true;
}
