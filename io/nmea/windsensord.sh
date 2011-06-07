#!/bin/bash
#
# Copyright 2011 The Avalon Project Authors. All rights reserved.
# Use of this source code is governed by the Apache License 2.0
# that can be found in the LICENSE file.
#
# Example command-line to start the windsensor daemon.

NMEAD="./nmead"

if [[ $# != 3 ]]; then
  echo "Usage: $0 <windsensor tty device> <windsensor ouput file> " \
    "<temperature ouput file>"
  exit 1
fi

TTY_DEVICE=$1
WIMWV_OUTPUT=$2
WIXDR_OUTPUT=$3
BAUDRATE=4800

$NMEAD $TTY_DEVICE $BAUDRATE "WIMWV:$WIMWV_OUTPUT" "WIXDR:$WIXDR_OUTPUT"
