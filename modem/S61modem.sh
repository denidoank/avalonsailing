#! /bin/sh

set -e

DESC="Iridium Sat modem"
DIR=/usr/bin
PHONE=41763038610
DEVICE=/dev/ttyUSB8
QUEUE=/tmp/iridium


case "$1" in
  start)
        echo -n "Starting $DESC: "
	test -d $QUEUE || mkdir -p $QUEUE
	$DIR/modemd --no-syslog --task=modemd --timeout=60 --debug --device=$DEVICE --phone=$PHONE --queue=$QUEUE | $DIR/plug -i /var/run/lbus >/dev/null 2>&1 &
        echo "OK"
        ;;
  stop)
        echo -n "Stopping $DESC: "
	kill `cat /var/run/modem.pid`
	killall modemd
        echo "OK"
        ;;
  restart|force-reload)
        echo "Restarting $DESC: "
        $0 stop
        sleep 1
        $0 start
        echo ""
        ;;
  *)
        echo "Usage: $0 {start|stop|restart|force-reload}" >&2
        exit 1
        ;;
esac

exit 0
