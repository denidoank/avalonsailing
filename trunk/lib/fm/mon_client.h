// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LOG_FM_MONCLIENT_H__
#define LOG_FM_MONCLIENT_H__

#include <sys/types.h>
#include <sys/socket.h>

// Client interface for sending reports to system monitor task
class MonClient {
 public:
  MonClient();
  ~MonClient();
  void SetDestination(const char *address);
  const char *GetDestination() const { return monitor_address_.sa_data; }
  void SendMonMsg(const char *fmt, ...) const
      __attribute__((format(printf, 2, 3)));
 private:
  int sockfd_;
  sockaddr monitor_address_;
};

#endif //  LOG_FM_MONCLIENT_H__
