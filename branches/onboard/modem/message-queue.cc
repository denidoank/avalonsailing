// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "message-queue.h"

#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <libgen.h>
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#define DEFAULT_PERMISSIONS (S_IRUSR | S_IWUSR)
#define DEFAULT_DIR_PERMISSIONS (S_IFDIR | S_IRWXU)

// SMS message spooler and receiver API.
MessageQueue::MessageQueue(const string base_dir): base_dir_(base_dir) {
  list<string> dirs;
  string dir = base_dir;
  do {  // Create a list of parent directories.
    dirs.push_back(dir);
    char dir_c_str[256];
    strncpy(dir_c_str, dir.c_str(), sizeof(dir_c_str) - 1);
    dir.assign(dirname(dir_c_str));
  } while (dirs.back() == dir);
  while (!dirs.empty()) {  // Create directories that do not exist.
    mkdir(dirs.back().c_str(), DEFAULT_DIR_PERMISSIONS);
    dirs.pop_back();
  }
}

MessageQueue:: ~MessageQueue() {
}

void MessageQueue::EmptyQueue() {
  vector<MessageId> ids;
  GetAvailableIds(ids);
  // Delete all messages.
  for (unsigned int i = 0; i < ids.size(); ++i) {
    DeleteMessage(ids[i]);
  }
}

unsigned int MessageQueue::NumMessages() {
  vector<MessageId> ids;
  GetAvailableIds(ids);
  return ids.size();
}

MessageQueue::MessageId MessageQueue::GetMessage(
    const unsigned int index, string* message) {
  message->clear();
  vector<MessageId> ids;
  MessageId id = kInvalidId;
  GetAvailableIds(ids);
  if (index >= 0 && index < ids.size()) {
    id = ids[index];
    // Open message on disk.
    const string filename = GetFilenameFromId(id);
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
      syslog(LOG_ERR, "Error opening message id %u.", id);
      id = kInvalidId;      
    } else {
      char buffer[1024];
      int length = 0;
      while ((length = read(fd, buffer, sizeof(buffer))) ==
             sizeof(buffer)) {
        message->append(buffer, length);
      }
      if (length < 0) {
        syslog(LOG_ERR, "Error reading message id %u.", id);
        id = kInvalidId;
      } else {
        syslog(LOG_INFO, "Opening message id %u.", id);
        message->append(buffer, length);
      }
    }
    close(fd);
  }
  return id;
}

bool MessageQueue::DeleteMessage(const MessageId message_id) {
  string filename = GetFilenameFromId(message_id);
  syslog(LOG_INFO, "Remove message id %u.", message_id);
  if (unlink(filename.c_str())) {
    syslog(LOG_ERR, "Removing message id %u failed.", message_id);    
    return false;
  }
  return true;
}

MessageQueue::MessageId MessageQueue::PushMessage(const string& message) {
  char temp_filename[256] = "";
  strncat(temp_filename, base_dir_.c_str(), sizeof(temp_filename) - 1);
  strncat(temp_filename, "/temp_XXXXXX", sizeof(temp_filename) - 1);
  int fd = mkstemp(temp_filename);
  if (fd < 0) {
    syslog(LOG_ERR, "Error creating message (temp file: %s).", temp_filename);
    return kInvalidId;
  }
  if (write(fd, message.c_str(), message.length()) !=
      static_cast<int>(message.length())) {
    syslog(LOG_ERR, "Error writing message (temp file: %s).", temp_filename);
    close(fd);
    unlink(temp_filename);
    return kInvalidId;
  }
  close(fd);  
  // Atomic rename of the temporary file.
  MessageId id = GetNextId();  
  string filename = GetFilenameFromId(id);
  if (rename(temp_filename, filename.c_str())) {
    syslog(LOG_ERR, "Error pushing message id: %d (file: %s)",
           id, filename.c_str());
    unlink(temp_filename);
    return kInvalidId;
  } 
  return id;
}

MessageQueue::MessageId MessageQueue::GetNextId() const {
  // Message IDs should be uniquely identifiers. Therefore message id is
  // generated using process id as the most significant part.
  unsigned int first_pid_id = (static_cast<unsigned int>(getpid())) << 16;
  MessageId id = first_pid_id;
  vector<MessageId> ids;
  GetAvailableIds(ids);
  for (unsigned int i = 0; i < ids.size(); ++i) {
    if (first_pid_id == (ids[i] >> 16) << 16)
      id = ids[i];  // Update with latest pid based id.
  }
  id++; // Increment last valid id based on this pid.
  if (first_pid_id != (id >> 16) << 16) {
    // We overflowed 16bit space. Restarting from first pid based id.
    id = first_pid_id;
  }
  return id;
}

void MessageQueue::GetAvailableIds(vector<MessageId>& ids) const {
  vector<pair<time_t, MessageId> > sorted_ids;
  struct dirent **namelist;
  int n;
  // List all files in the queue directory.
  n = scandir(base_dir_.c_str(), &namelist, 0, 0);
  if (n < 0) {
    syslog(LOG_CRIT, "Cannot scan message queue dir: %s", base_dir_.c_str());
    exit(1);
  } else {
    while (n--) {  // For each file entry.
      MessageId id = GetIdFromFilename(namelist[n]->d_name);
      if (id != kInvalidId) {  // If file is matching a message filepattern.
        struct stat file_stat;
        // Get file creation time.
        stat((base_dir_ + "/" + namelist[n]->d_name).c_str(), &file_stat);
        sorted_ids.push_back(make_pair(file_stat.st_ctime, id));
      }
      free(namelist[n]);
    }
    free(namelist);
  }
  // Sort all messages: first by creation time and then by message id.
  sort(sorted_ids.begin(), sorted_ids.end());
  for(vector<pair<time_t, MessageId> >::const_iterator it = sorted_ids.begin();
      it != sorted_ids.end(); ++it) {
    ids.push_back(it->second);
  }
}

MessageQueue::MessageId MessageQueue::GetIdFromFilename(
    const char* filename) {
  MessageId id = kInvalidId;
  if (fnmatch("message.[0-9]*.txt", filename, FNM_CASEFOLD) == 0) {
    char* endptr;
    id = strtol(filename + 8, &endptr, 10);  // Extract message id.
    if (!endptr || *endptr != '.') {
      id = kInvalidId;
    }    
  }
  return id;
}

string MessageQueue::GetFilenameFromId(const MessageId message_id) const {
  char filename[256];
  snprintf(filename, sizeof(filename) - 1,
           "%s/message.%u.txt", base_dir_.c_str(), message_id);
  return filename;
}
