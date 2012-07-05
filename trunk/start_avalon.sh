#!/bin/sh
#
# Copyright 2011 The Avalon Project Authors. All rights reserved.
# Use of this source code is governed by the Apache License 2.0
# that can be found in the LICENSE file.
#
## START AVALON
##
## Script to start all subsystems
##

# for experimental use, get the binaries directly from the source tree.
if ! which linebusd; then
    echo "Running from source tree"
    AVALONROOT=$(dirname $0)
    export PATH=$AVALONROOT/io:$AVALONROOT/io/rudderd2:$AVALONROOT/helmsman:$AVALONROOT/systools:$PATH

    EBUS=/tmp/ebus
    LBUS=/tmp/lbus

else

    EBUS=/var/run/ebus
    LBUS=/var/run/lbus

fi

err=""
for p in linebusd plug eposprobe ruddersts eposmon eposcom rudderctl sailctl drivests skewmon \
	imucfg imucat aiscat aisbuf compasscat windcat fcmon; do
    which $p >/dev/null 2>&1 || err="$err $p"
done

if [ "$err" != "" ]; then
    echo "Missing binaries: $err"
    echo "quitting."
    exit 1
fi


# the ports you get depend on the order the imu and the 8-port are detected
# TODO don't use the imu, or fall back to the windsensor or the compass
if imucfg /dev/ttyUSB0; then

    let i=0
    for d in imu rudder_l rudder_r sail fuelcell compass ais modem wind; do
	ln -fs /dev/ttyUSB$i /dev/$d
	let i=$i+1
    done

elif imucfg /dev/ttyUSB8; then

    let i=0
    for d in rudder_l rudder_r sail fuelcell compass ais modem wind imu; do
	ln -fs /dev/ttyUSB$i /dev/$d
	let i=$i+1
    done

else

    echo "No IMU found on ttyUSB0 or ttyUSB8"
    echo "quitting."
    exit 1

fi


#
# General setup: the lbus has the low frequency/high level communication (ais, compass, helmsman etc)
# the ebus does the high frequency/low latency low level epos commands in ebus.h protocol (mostly)
# programs are connected to the bus with the plug program.
# it's options:
#
#    -i  only take the output of the program as input to the bus (i.e. dont receive any messages from the bus)
#    -o  only take what comes from the bus and feed it to the program (i.e. dont send any messages to the bus)
#    -n name  a name for debugging when you ask for stats with kill -USR1 $bus.pid, not (yet) relevant for -i plugs.
#    -f instal a filter on prefix (not relevant for -i plugs).
#    -p tell the linebusd this one is 'precious': if it exits, take down the bus
#
#  programs like eposcom and sailctl that are connected to the bus with both stdin and stdout typically install their own filters
#  on the linebus, so there's no need to specify them there.
#  the -- is needed in plug [options] $BUS -- command [command options] to explain to plug where to stop interpreting its options.
#

##    modemd  --queue=/var/run/modem --device=/dev/modem &

for attempt in 1 2 3; do

    logger -s -p local2.Notice "Avalon subsystems starting up (attempt $attempt)"

    kill `cat $LBUS.pid` 2> /dev/null 
    linebusd $LBUS	# blocks until socket exists, then goes daemon

    # input subsystems

    plug -pi $LBUS -- `which imucat` /dev/imu
    plug -pi $LBUS -- `which compasscat` /dev/compass
    plug -pi $LBUS -- `which windcat` /dev/wind

    plug -po -n "imutime" -f "imu:" $LBUS -- `which imutime`
    plug -i $LBUS -- `which fcmon` /dev/fuelcell


    # output subsystems

    kill `cat $EBUS.pid` 2> /dev/null 
    linebusd $EBUS	# blocks until socket exists, then goes daemon

    for p in /dev/rudder_l /dev/rudder_r /dev/sail;  do
	    plug -n "epos-$(basename $p)" $EBUS -- `which eposcom` -T $p &
    done

    plug -p $EBUS -- `which rudderctl` -l -T &		# homing and positioning of left rudder
    plug -p $EBUS -- `which rudderctl` -r -T &		# homing and positioning of right rudder
    plug -p $EBUS -- `which skewmon` -T &		# monitor sail motor/bmmh sensor angle skew
    plug -p $EBUS -- `which sailctl` -T &		# positioning for sail
    plug -pi $EBUS -- `which eposprobe` -f 2 -T &    # periodically (-f Hz) issue status register probe commands (needed by drivests and eposmon)
    plug -o -n "eposmon" $EBUS -- `which eposmon` & # summarize and report epos communication errors to syslog

    #   drivests: decode ebus status registers to lbus status_: messages
    #   plug -f rudderctl: forward rudderctl: messages to ebus
    plug -po -n "drivests" $EBUS | drivests -n 100 | plug -pn "rudderctlfwd" -f "rudderctl:" $LBUS | plug -pi $EBUS &

    
    plug -n "helmsman" $LBUS -- `which helmsman` 2>> /var/log/helmsman.log & 



    # aiscat is not connected to the lbus
    #  (aiscat /dev/ais | aisbuf /var/run/ais.txt; logger -s -p local2.crit "aiscat exited.")&

    # plug -o -n "statusd" $LBUS -- `which statusd` --queue=/var/run/modem --initial_timeout=180 --status_interval=86400 --remote_cmd_interval=5 &

    echo "Und immer eine Handbreit Wasser unter dem Kiel!"
    echo "May she always have a good passage, wherever she goes!"
    echo

    ## keep a plug with a dummy filter on the lbus and wait for it to exit
    plug -n "watch" -f xxx $LBUS > /dev/null

    imucfg -R /dev/imu || logger -s -p local2.Notice "Failed to reset IMU"

done


logger -s -p local2.crit "Main loop ran 3 times, time to reboot ...."

# in production we'd send an sms here and reboot
