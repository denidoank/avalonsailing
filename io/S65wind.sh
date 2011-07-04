#! /bin/sh

set -e

DESC="avalon Wind sensor"
DIR=/usr/bin

case "$1" in
  start)
        echo -n "Starting $DESC: "
	$DIR/nmeacat /dev/ttyUSB1 | $DIR/gulp /var/run/wind >/dev/null 2>&1 &
        echo "OK"
        ;;
  stop)
        echo -n "Stopping $DESC: "
	kill `cat /var/run/wind.pid`
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
