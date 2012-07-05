#!/bin/sh
#
# Copyright 2011 The Avalon Project Authors. All rights reserved.
# Use of this source code is governed by the Apache License 2.0
# that can be found in the LICENSE file.
#

# for experimental use, get the binaries directly from the source tree.
if ! which linebusd; then
    echo "Running from source tree"
    AVALONROOT=$(dirname $0)
    export PATH=$AVALONROOT/io:$AVALONROOT/io/rudderd2:$AVALONROOT/helmsman:$AVALONROOT/systools:$PATH

    EBUS=/tmp/ebus

else

    EBUS=/var/run/ebus

fi

err=""
for p in linebusd plug eposprobe ruddersts eposmon eposcom rudderctl sailctl drivests skewmon imucfg; do
    which $p >/dev/null 2>&1 || err="$err $p"
done

if [ "$err" != "" ]; then
    echo "Missing binaries: $err"
    echo "quitting."
    exit 1
fi


# the ports you get depend on the order the imu and the 8-port are detected

if imucfg /dev/ttyUSB0; then

    PORT_RUDDER_L=/dev/ttyUSB1
    PORT_RUDDER_R=/dev/ttyUSB2
    PORT_SAIL=/dev/ttyUSB3

elif imucfg /dev/ttyUSB8; then

    PORT_RUDDER_L=/dev/ttyUSB0
    PORT_RUDDER_R=/dev/ttyUSB1
    PORT_SAIL=/dev/ttyUSB2

else

    echo "No IMU found on ttyUSB0 or ttyUSB8, can't determine port order."
    echo "quitting."
    exit 1

fi

linebusd $EBUS	# blocks until socket exists

logger -s -p local2.notice "Starting epos subsystems"

for p in $PORT_RUDDER_L  $PORT_RUDDER_R
do
    (plug -n "epos-$(basename $p)" $EBUS -- `which eposcom` -T $p; logger -s -p local2.crit "Eposcom $p exited.")&
done

(plug $EBUS -- `which rudderctl` -l -T; logger  -s -p local2.crit "Rudderctl (LEFT) exited.")&
(plug $EBUS -- `which rudderctl` -r -T; logger  -s -p local2.crit "Rudderctl (RIGHT) exited.")&

plug -i $EBUS -- `which eposprobe` -f 2 -T &    # periodically (-f Hz) issue status register probe commands (needed by ruddersts and -mon)
plug -o -n "eposmon" $EBUS -- `which eposmon` & # summarize and report epos communication errors to syslog

(while /bin/true; do
    echo 'rudderctl: timestamp_ms:0 rudder_l_deg:10 rudder_r_deg:10 sail_deg:-10'
    sleep 5
    echo 'rudderctl: timestamp_ms:0 rudder_l_deg:-10 rudder_r_deg:-10 sail_deg:10'
    sleep 5
done) | plug -i $EBUS