// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SMS_LOCKFILE_H
#define SMS_LOCKFILE_H

// if the lockfile exists, check if it is stale.
// if it doesn't exist, or it is stale replace it with our own PID.
// returns true if we got the lock.
int lockfile(const char* path);

// if the lockfile exists and contains our own pid, remove it.
// returns false if someone else stole the lock.
int unlockfile(const char* path);

#endif // SMS_LOCKFILE_H
