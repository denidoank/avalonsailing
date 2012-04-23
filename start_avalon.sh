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

EBUS=/tmp/ebus  # /var/run/ebus
LBUS=/tmp/lbus  # /var/run/lbus

export PATH=/tmp/bin:$PATH  # TODO
#cd /var/run
cd /tmp

err=""
for p in linebusd plug epos probe ruddersts rudderctlfwd eposmon eposcom rudderctl sailctl\
	imucfg imucat aiscat aisbuf compasscat windcat fcmon; do
    which $p >/dev/null 2>&1 || err="$err $p"
done

if [ "$err" != "" ]; then
    echo "Missing binaries: $err"
    echo "quitting."
    exit 1
fi

logger -s -p local2.Notice "Avalon subsystems starting up"

linebusd $LBUS

while /bin/true
do
    logger -s -p local2.Notice "(Re)Starting epos subsystems"

    linebusd $EBUS	# blocks until socket exists

    for p in $PORT_RUDDER_L  $PORT_RUDDER_R $PORT_SAIL 
    do
	(
	    plug $EBUS -- eposcom $p
	    logger -s -p local2.crit "Eposcom $p exited."
	    kill `cat $EBUS.pid`
	)&
    done

    (
	plug $EBUS -- rudderctl -l		# homing and positioning of left rudder
	logger  -s -p local2.crit "Rudderctl -l exited."
	kill `cat $EBUS.pid`
    )&

    (
	plug $EBUS -- rudderctl -r		# homing and positioning of right rudder
	logger  -s -p local2.crit "Rudderctl -r exited."
	kill `cat $EBUS.pid`
    )&

    (
	plug $EBUS --  sailctl  &		# positioning for sail
	logger  -s -p local2.crit "Sailctl exited."
	kill `cat $EBUS.pid`
    )&

    # eposprobe: periodically issue status register probe commands (needed by ruddersts and -mon)
    # ruddersts: decode status registers to ruddersts: messages
    # rudderctlfwd: forward rudderctl: messages to ebus, prefixed with '#'
    # eposmon: summarize and report epos communication errors to syslog
    eposprobe | plug $EBUS | ruddersts | plug $LBUS | rudderctlfwd | plug $EBUS | eposmon
    logger -s -p local2.crit "Epos status subsystem exited."
    kill `cat $EBUS.pid`

    sleep 5
done

exit  # for now

(imucfg $PORT_IMU && imucat $PORT_IMU | ./plug -i $LBUS ; logger  -s -p local2.crit "imucat exited.")&
(compasscat $PORT_COMPASS | plug -i $LBUS;  logger  -s -p local2.crit "compasscat exited.")&
(windcat $PORT_WIND | plug -i $LBUS;  logger -s -p local2.crit "windcat exited.")&
(fcmon $PORT_FUELCELL | plug -i $LBUS ;  logger -s -p local2.crit "fcmon exited.")&

# (aiscat $PORT_AIS | aisbuf /var/run/ais.txt; logger -s -p local2.crit "aiscat exited.")&

plug $LBUS -- helmsman 2>&1 | logger -p local2.debug

# modemd --device=$PORT_MODEM --queue=/var/run/modem | plug $LBUS | statusd  --queue=/var/run/modem \
#	--initial_timeout=180 --status_interval=86400 --remote_cmd_interval=5 

echo "Und immer eine Handbreit Wasser unter dem Kiel!"
echo "May she always have a good passage, wherever she goes!"
