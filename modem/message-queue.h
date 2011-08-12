// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef MODEM_MESSAGE_QUEUE_H_
#define MODEM_MESSAGE_QUEUE_H_
 
#include <string>
#include <vector>

using namespace std;

// SMS message spooler and receiver API.
class MessageQueue {
 public:
  typedef unsigned int MessageId;
  static const MessageId kInvalidId = 0;

  // Constructs a message queue using specified directory for storing messages.
  MessageQueue(const string base_dir);
  ~MessageQueue();

  // Empty message queue. Remove all messages.
  void EmptyQueue();

  // Returns the number of messages in the queue.
  unsigned int NumMessages();

  // Read message from the inbox from position index (between 0 to
  // NumMessages). First position corresponds to oldest pushed messages where
  // last position corresponds with the newest message. Returns a valid message
  // id, if successful, or kInvalidId otherwise.
  MessageId GetMessage(const unsigned int index, string* message);

  // Delete message with corresponding id. Returns true if successfull.
  bool DeleteMessage(const MessageId message_id);

  // Push message in the queue. Returns an unique message id, if successfull, or
  // kInvalidId otherwise.
  MessageId PushMessage(const string& message);

 private:
  string base_dir_;

  static MessageId GetIdFromFilename(const char* filename);
  void GetAvailableIds(vector<MessageId>& ids) const;
  MessageId GetNextId() const;
  string GetFilenameFromId(const MessageId message_id) const;
};

#endif  // MODEM_MESSAGE_QUEUE_H_
