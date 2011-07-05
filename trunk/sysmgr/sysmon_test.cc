// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "sysmgr/sysmon.h"

#include <stdio.h>

#include "lib/fm/fm.h"
#include "sysmgr/process_table.h"

// Testing wrapper for sysmon

int main(int argc, char **argv) {
  const char *argv0 = argv[0];
  int optind = FM::Init(argc, argv);
  argc -= optind;
  argv += optind;

  if (argc != 2) {
    fprintf(stderr, "usage: %s task.conf sysmon.conf\n", argv0);
    return 1;
  }
  ProcessTable proc_table;
  proc_table.Load(argv[0]);

  SysMon sysmon(FM::GetTimeoutS(), 2 /* stderr */, argv[1], proc_table);
  sysmon.Run();

}
