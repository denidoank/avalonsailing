#!/bin/bash

# Copyright 2011 The Avalon Project Authors. All rights reserved.
# Use of this source code is governed by the Apache License 2.0
# that can be found in the LICENSE file.
#
# START AVALON
#
# Script to start all subsystems
#


/usr/bin/linebusd /var/run/lbus

/usr/bin/eposd /dev/ttyUSB2 /dev/ttyUSB3 /dev/ttyUSB4

/usr/bin/plug /var/run/lbus /usr/bin/rudderd /var/run/eposd

/usr/bin/imucat  /dev/xsensIMU   | /usr/bin/plug -i /var/run/lbus &
/usr/bin/windcat /dev/windsensor | /usr/bin/plug -i /var/run/lbus &
/usr/bin/aiscat  /dev/ttyUSB7    | /usr/bin/aisbuf /var/run/ais.txt &
/usr/bin/fcmon   /dev/ttyUSB5    | /usr/bin/plug -i /var/run/lbus &

/usr/bin/plug /var/run/lbus /usr/bin/helmsman 2>/dev/null

/usr/bin/modemd --device=/dev/ttyUSB8 --queue=/var/run/modem | /usr/bin/plug -i /var/run/lbus 2>/dev/null &

/usr/bin/plug -o /var/run/lbus | /usr/bin/statusd  --queue=/var/run/modem \
--initial_timeout=180 --status_interval=86400 --remote_cmd_interval=5 >/dev/null 2>&1 &

echo "Und immer eine Handbreit Wasser unter dem Kiel!"
echo "May she always have a good passage, wherever she goes!"
