#!/bin/bash
#
# Copyright 2011 The Avalon Project Authors. All rights reserved.
# Use of this source code is governed by the Apache License 2.0
# that can be found in the LICENSE file.
#
## START AVALON
##
## Script to start all subsystems
##

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

# for experimental use, get the binaries directly from the source tree.
if !which linebusd; then
    AVALONROOT=/home/avalon/avalonsailing
    export PATH=$AVALONROOT/io:$AVALONROOT/io/rudderd2:$AVALONROOT/helmsman:$AVALONROOT/systools:$PATH
fi

err=""
for p in linebusd plug eposprobe ruddersts rudderctlfwd eposmon eposcom rudderctl sailctl\
	imucfg imucat aiscat aisbuf compasscat windcat fcmon; do
    which $p >/dev/null 2>&1 || err="$err $p"
done

if [ "$err" != "" ]; then
    echo "Missing binaries: $err"
    echo "quitting."
    exit 1
fi

logger -s -p local2.Notice "Avalon subsystems starting up"

linebusd $LBUS	# blocks until socket exists
echo "to stop all avalon systems"
echo "   kill \`cat $LBUS.pid\`"

# keep restarting rudder suite until LBUS is gone.
# plug will return false in that case. the echo is to make it exit if it does connect.
(while echo | plug $LBUS > /dev/null 2>&1 ; do

    kill `cat $EBUS.pid` 2> /dev/null 
    linebusd $EBUS	# blocks until socket exists

    logger -s -p local2.notice "(Re)Starting epos subsystems"

    for p in $PORT_RUDDER_L  $PORT_RUDDER_R $PORT_SAIL 
    do
	(
	    plug $EBUS -- `which eposcom` $p
	    logger -s -p local2.crit "Eposcom $p exited."
	    kill `cat $EBUS.pid`
	)&
    done

    (
	plug $EBUS -- `which rudderctl` -l		# homing and positioning of left rudder
	logger  -s -p local2.crit "Rudderctl (LEFT) exited."
	kill `cat $EBUS.pid`
    )&

    (
	plug $EBUS -- `which rudderctl` -r		# homing and positioning of right rudder
	logger  -s -p local2.crit "Rudderctl (RIGHT) exited."
	kill `cat $EBUS.pid`
    )&

    (
	plug $EBUS --  `which sailctl`			# positioning for sail
	logger  -s -p local2.crit "Sailctl exited."
	kill `cat $EBUS.pid`
    )&

    # eposprobe: periodically issue status register probe commands (needed by ruddersts and -mon)
    # ruddersts: decode status registers to ruddersts: messages
    # rudderctlfwd: forward rudderctl: messages to ebus, prefixed with '#'
    # eposmon: summarize and report epos communication errors to syslog
    eposprobe -f8 | plug $EBUS | ruddersts -n100 | plug $LBUS | rudderctlfwd | plug $EBUS | eposmon
    logger -s -p local2.crit "Epos status subsystem exited."
    kill `cat $EBUS.pid`

done

    logger -s -p local2.crit "Epos subsystems NOT RESTARTING (lbus pidfile gone)"
)&

# todo: each of these in a restart loop (with rate limiting)
(imucfg $PORT_IMU &&
    imucat $PORT_IMU 		| plug -i $LBUS; logger -s -p local2.crit "imucat exited.")&
(compasscat $PORT_COMPASS 	| plug -i $LBUS; logger -s -p local2.crit "compasscat exited.")&
(windcat $PORT_WIND 		| plug -i $LBUS; logger -s -p local2.crit "windcat exited.")&
#(fcmon $PORT_FUELCELL 		| plug -i $LBUS; logger -s -p local2.crit "fcmon exited.")&

# (aiscat $PORT_AIS | aisbuf /var/run/ais.txt; logger -s -p local2.crit "aiscat exited.")&

# currently helmsman goes daemon. maybe make it not do that?
plug $LBUS -- `which helmsman` 2>&1 | logger -p local2.debug

# modemd --device=$PORT_MODEM --queue=/var/run/modem | plug $LBUS | statusd  --queue=/var/run/modem \
#	--initial_timeout=180 --status_interval=86400 --remote_cmd_interval=5 

echo "Und immer eine Handbreit Wasser unter dem Kiel!"
echo "May she always have a good passage, wherever she goes!"
