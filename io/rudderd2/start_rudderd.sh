#!/bin/sh
export PATH=.
LBUS=/var/run/lbus # (linebusd $LBUS should run already)

EBUS=/var/run/ebus
linebusd $EBUS

plug $EBUS eposcom /dev/ttyUSB2  &  # first rudder epos
plug $EBUS eposcom /dev/ttyUSB3  &  # second rudder epos
plug $EBUS eposcom /dev/ttyUSB4  &  # sail epos

eposprobe | plug -o $EBUS &	# periodically issue status register probe commands
plug -o $EBUS | eposmon		# summarize and report epos communication errors to syslog

plug $EBUS rudderctl -l &  # left rudder       # homing and positioning of rudder
plug $EBUS rudderctl -r &  # right rudder      # same
plug $EBUS sailctl  &      # sail              # positioning for sail

if /bin/test -r $LBUS; then
    plug -o $EBUS | ruddersts | plug -i $LBUS & # decode status registers to ruddersts: messages
    plug -o $LBUS | rudderctlfwd | plug -i $EBUS &  # forward rudderctl: messages to ebus, prefixed with '#'
fi
