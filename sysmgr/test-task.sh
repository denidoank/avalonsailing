#!/bin/bash
# Copyright 2011 The Avalon Project Authors. All rights reserved.
# Use of this source code is governed by the Apache License 2.0
# that can be found in the LICENSE file.

# Dummy workload for testing sysmgr.

on_exit() {
 echo "terminating $name"
 exit 0
}

trap 'on_exit' 2 15

name=$1
echo "starting $name"

while true; do
 sleep 10
 if  [ $# -gt 1 ]; then
   if [ $2 = "crash" ]; then
     echo "crashing $name"
     exit 1
   fi
 fi
done

