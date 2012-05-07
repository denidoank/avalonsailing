#!/bin/sh

# script for testing on the laptop from rudderd2 directory
T=-T   # enable timing on ebus

LBUS=/tmp/lbus			# (linebusd $LBUS should run already)
EBUS=/tmp/ebus

echo "Epos subsystem starting up"

../linebusd $EBUS		# will background itself when the socket is ready

# periodically issue status register probe commands (needed by ruddersts and -mon)
# summarize and report epos communication errors to syslog
../plug -i ./eposprobe $T
../plug -o -n 'eposprobe/mon' $EBUS ./eposmon &

../plug -n 'epos1' $EBUS ./eposcom $T /dev/ttyUSB1  &	# first rudder epos
../plug -n 'epos2' $EBUS ./eposcom $T /dev/ttyUSB2  &	# second rudder epos
../plug -n 'epos3' $EBUS ./eposcom $T /dev/ttyUSB3  &	# sail epos

../plug $EBUS ./rudderctl $T -l &	# homing and positioning of left rudder
../plug $EBUS ./rudderctl $T -r &	# same for right
../plug $EBUS ./sailctl   $T &	# positioning for sail

if /usr/bin/test -r $LBUS; then
	# decode status registers to ruddersts: messages
	# forward rudderctl: messages to ebus
    ../plug -o -n 'ruddersts' $EBUS | ./ruddersts | ../plug -n 'rudderctlfwd' -f 'rudderctl:' $LBUS | ../plug -i $EBUS &
	echo To stop test:
	echo "  kill `cat $EBUS.pid`"

else
    ../plug -o -n 'ruddersts' $EBUS | ./ruddersts
    echo killing ebus
    kill `cat $EBUS.pid`
fi

