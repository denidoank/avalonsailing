#!/bin/sh

PATH=`pwd`:/bin:/usr/bin
export PATH

LBUS=/var/run/lbus			# (linebusd $LBUS should run already)
EBUS=/var/run/ebus

logger -s -p local2.Notice "Epos subsystem starting up"

linebusd $EBUS				# will background itself when the socket is ready

eposprobe | plug -i $EBUS &		# periodically issue status register probe commands (needed by ruddersts and -mon)
plug -o $EBUS | eposmon &		# summarize and report epos communication errors to syslog

plug $EBUS -- eposcom /dev/ttyUSB1  &	# first rudder epos
plug $EBUS -- eposcom /dev/ttyUSB2  &	# second rudder epos
plug $EBUS -- eposcom /dev/ttyUSB3  &	# sail epos

plug $EBUS -- rudderctl -l &		# homing and positioning of left rudder
plug $EBUS -- rudderctl -r &		# same for right
plug $EBUS -- sailctl  &		# positioning for sail

if /usr/bin/test -r $LBUS; then
    plug -o $EBUS | ruddersts    | plug -i $LBUS &	# decode status registers to ruddersts: messages
    plug -o $LBUS | rudderctlfwd | plug -i $EBUS &	# forward rudderctl: messages to ebus, prefixed with '#'
fi

# if anything dies, kill the EBUS, which will kill everything else
wait
kill `cat $EBUS.pid`
logger -s -p local2.crit "Epos subsystem terminated"