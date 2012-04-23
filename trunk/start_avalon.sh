#!/bin/bash

# Copyright 2011 The Avalon Project Authors. All rights reserved.
# Use of this source code is governed by the Apache License 2.0
# that can be found in the LICENSE file.
#
# START AVALON
#
# Script to start all subsystems
#

PORT_RUDDER_L=/dev/ttyUSB0
PORT_RUDDER_R=/dev/ttyUSB1
PORT_SAIL=/dev/ttyUSB2
PORT_FUELCELL=/dev/ttyUSB3
PORT_COMPASS=/dev/ttyUSB4
PORT_AIS=/dev/ttyUSB5
PORT_MODEM=/dev/ttyUSB6
PORT_WIND=/dev/ttyUSB7
PORT_IMU=/dev/ttyUSB8	#  /dev/xsensIMU

#cd /var/run
cd /tmp

logger -s -p local2.Notice "Avalon subsystems starting up"

linebusd ./lbus
linebusd ./ebus

while /bin/true
do

    (
	# eposprobe: periodically issue status register probe commands (needed by ruddersts and -mon)
	# ruddersts: decode status registers to ruddersts: messages
	# rudderctlfwd: forward rudderctl: messages to ebus, prefixed with '#'
	# eposmon: summarize and report epos communication errors to syslog
	eposprobe | plug $EBUS | ruddersts | plug $LBUS | rudderctlfwd | plug $EBUS | eposmon
	logger -s -p local2.crit "Epos status subsystem exited."
	kill `cat $EBUS.pid`
    )&

    for p in $PORT_RUDDER_L  $PORT_RUDDER_R $PORT_SAIL 
    do
	(
	    plug $EBUS -- eposcom $p
	    logger -s -p local2.crit "Eposcom $p exited."
	    kill `cat $EBUS.pid`
	)&
    done

    (while /bin/true; do
	plug $EBUS -- rudderctl -l		# homing and positioning of left rudder
	logger  -s -p local2.crit "Rudderctl -l exited."
	sleep 5
    done) &

    (while /bin/true; do
	plug $EBUS -- rudderctl -r		# homing and positioning of right rudder
	logger  -s -p local2.crit "Rudderctl -r exited."
	sleep 5
    done) &

    (while /bin/true; do
	plug $EBUS --  sailctl  &		# positioning for sail
	logger  -s -p local2.crit "Sailctl exited."
	sleep 5
    done) &

done


exit  # for now

imucfg && imucat /dev/xsensIMU | plug -i /var/run/lbus &

windcat  | plug -i /var/run/lbus &
aiscat  /dev/ttyUSB7    | aisbuf /var/run/ais.txt &
fcmon   /dev/ttyUSB5    | plug -i /var/run/lbus &

plug /var/run/lbus helmsman 2>/dev/null

modemd --device=/dev/ttyUSB8 --queue=/var/run/modem | plug -i /var/run/lbus 2>/dev/null &

plug -o /var/run/lbus | statusd  --queue=/var/run/modem \
--initial_timeout=180 --status_interval=86400 --remote_cmd_interval=5 >/dev/null 2>&1 &

echo "Und immer eine Handbreit Wasser unter dem Kiel!"
echo "May she always have a good passage, wherever she goes!"
