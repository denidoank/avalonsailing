#!/bin/sh
#
# Script to restart some components on the lbus with buswatch.  supposed to run with a timeout of 30-60 seconds.
# a separate script should handle powercycling and rebooting after, say 300-600 second timeouts.
# if the lbus itself fails, that should be handled elsewhere by a plug -fxxx exiting.
#
E=echo

while [ -n "$1" ]; do
    case "$1" in 
	ais:|compass:|fuelcell:|gps:|imu:|wind:)
	    $E /etc/init.d/$(echo $1|tr -d :) restart
	    ;;
	status_left:|status_right:|status_sail:)
	    $E /etc/init.d/ebus restart
	    ;;
	helmsman_st:)
	    $E /etc/init.d/helmsman restart
	    ;;
	helm:)
	    $E /etc/init.d/skipper restart
	    ;;
    esac
    shift
done

