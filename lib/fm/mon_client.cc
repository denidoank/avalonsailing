// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/fm/mon_client.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "lib/fm/log.h"

#define DEFAULT_SOCKET "/tmp/sysmon"

MonClient::MonClient() {
  SetDestination(DEFAULT_SOCKET);
  sockfd_ = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (sockfd_ == -1) {
    FM_LOG_PERROR("Opening monitoring client socket");
  } else {
    fcntl(sockfd_, F_SETFD, FD_CLOEXEC);
  }
}

MonClient::~MonClient() {
  if (sockfd_ != -1) {
    close(sockfd_);
  }
}

void MonClient::SetDestination(const char *address) {
  monitor_address_.sa_family = AF_UNIX;
  strncpy(monitor_address_.sa_data, address, sizeof(monitor_address_.sa_data));
}

void MonClient::SendMonMsg(const char *fmt, ...) const {
   va_list arg_ptr;
   char buffer[512];
   int len;

   va_start(arg_ptr, fmt);
   len = vsnprintf(buffer, sizeof(buffer), fmt, arg_ptr);
   va_end(arg_ptr);


   if (sendto(sockfd_, buffer, len + 1, 0,
              &monitor_address_, sizeof(monitor_address_)) == -1) {
     FM_LOG_PERROR("Sending report to sysmon");
   }
}
