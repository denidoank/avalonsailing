// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef LIB_FM_FM_MESSAGES_H__
#define LIB_FM_FM_MESSAGES_H__

#define FM_MSG_TASK_KEEPALIVE "name:%s time:%ld iter:%ld err:%ld warn:%ld"\
                               "info:%ld cpu:%ld mem:%ld"
#define FM_MSG_DEVICE_KEEPALIVE "name:%s time:%ld valid:%ld cerr:%ld hwerr:%ld"

#define FM_MSG_STATUS "name:%s status:%s msg:\"%s\""

#define FM_MSG_SIZE 512
#define FM_NAME_SIZE 30
#define FM_STATUS_SIZE 10

#endif // LIB_FM_FM_MESSAGES_H__
