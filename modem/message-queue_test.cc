// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/testing/testing.h"
#include "lib/testing/pass_fail.h"
#include "message-queue.h"

#define QUEUE_DIR     "/tmp/tmp-queue"
#define TEST_MESSAGE1 "test message 1"
#define TEST_MESSAGE2 "test message 2"

int main(int argc, char *argv[]) {
  // Empty queue.
  MessageQueue queue(QUEUE_DIR);
  queue.EmptyQueue();
  EXPECT_EQ(0, queue.NumMessages());
  
  // Add message 1.
  MessageQueue::MessageId id1 = queue.PushMessage(TEST_MESSAGE1);
  EXPECT_THAT(id1 != MessageQueue::kInvalidId);
  EXPECT_EQ(1, queue.NumMessages());

  // Add message 2.
  MessageQueue::MessageId id2 = queue.PushMessage(TEST_MESSAGE2);
  EXPECT_THAT(id2 != MessageQueue::kInvalidId);
  EXPECT_EQ(2, queue.NumMessages());

  // Check first message equals message 1.
  string message;
  EXPECT_EQ(id1, queue.GetMessage(0, &message));
  EXPECT_EQ(TEST_MESSAGE1, message);

  // Check next message equals message 2.
  EXPECT_EQ(id2, queue.GetMessage(1, &message));
  EXPECT_EQ(TEST_MESSAGE2, message);

  // Check next message does not exists.
  EXPECT_EQ(MessageQueue::kInvalidId, queue.GetMessage(2, &message));

  // Delete message 2.
  EXPECT_EQ(true, queue.DeleteMessage(id2));
  EXPECT_EQ(1, queue.NumMessages());
  EXPECT_EQ(id1, queue.GetMessage(0, &message));
  EXPECT_EQ(TEST_MESSAGE1, message);
  EXPECT_EQ(false, queue.DeleteMessage(id2));

  // Delete message 1.
  EXPECT_EQ(true, queue.DeleteMessage(id1));
  EXPECT_EQ(0, queue.NumMessages());
  EXPECT_EQ(MessageQueue::kInvalidId, queue.GetMessage(0, &message));

  PF_TEST(true, "Message queue test.");
}
