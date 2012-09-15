#!/bin/sh

PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin

sms recv > /tmp/sms.recv

[ -s /tmp/sms.recv ] || { logger "No sms'es received"; exit 0 }

logger "SMSes received: $(wc -l /tmp/sms.recv)"

# only honour the last one
tail -1 /tmp/sms.recv | read srcp pass cmd args

[ $pass == "1969" ] || { logger "received sms message without password: $(tail -1 /tmp/sms.recv)"; exit 0 }

case "$cmd" in 
    reboot)
	logger "Got SMS from $srcp to reboot"
	reboot
	;;

    setphone)
	logger "Got SMS from $srcp to add phonenumber: $args"
	mount -o remount,rw /
	cat "$args" >> /etc/smsphonenr
	mount -o remount,ro /
	;;
    
    setplan)
	logger "Got SMS from $srcp to set plan: $args"
	cat "$args" > /tmp/simple.txt
	;;

    smsstatus)
	logger "Got sms from $srcp status request"
	sms status | cut -c-155 > /tmp/sms.status
	for p in `cat /etc/smsphonenr`; do
	    sms send $p  "$(cat /tmp/sms.status)"
	done
	;;
esac

