#!/bin/bash

PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin

sms recv > /tmp/sms.recv

if [ ! -s /tmp/sms.recv ]; then
	logger "No sms'es received"
	exit 0
fi

logger "SMSes received: $(wc -l /tmp/sms.recv)"
# logger "received sms message : $(tail -1 /tmp/sms.recv)"

# only honour the last one
tail -1 /tmp/sms.recv | (read srcp pass cmd args
logger "received sms message src $srcp pwd $pass cmd $cmd args $args"

if [ -z "$pass" ] ; then 
	logger "received empty sms message : $(tail -1 /tmp/sms.recv)"
	exit 0
fi


if [ $pass != "1969" ] ; then
	logger "received sms message without password: $(tail -1 /tmp/sms.recv)"
	exit 0
fi

case "$cmd" in 
    reboot|RBT)
	logger "Got SMS from $srcp to reboot"
	reboot
	;;

    setphone|STPHN)
	logger "Got SMS from $srcp to add phonenumber: $args"
	mount -o remount,rw /
	echo "$args" >> /etc/smsphonenr
	mount -o remount,ro /
	;;
    
    setplan|STPLN)
	logger "Got SMS from $srcp to set plan: $args"
	echo "$args" > /tmp/simple.txt
	;;

    smsstatus|STS)
	logger "Got sms from $srcp status request"
	sms status | cut -c-155 > /tmp/sms.status
	for p in `cat /etc/smsphonenr`; do
	    sms send $p  "$(cat /tmp/sms.status)"
	done
	;;
     *)
	logger "Unknown sms command : $(tail -1 /tmp/sms.recv)"
	;;
esac
)
