#! /bin/sh

set -e

DESC="avalon IMU"
DIR=/usr/bin

case "$1" in
  start)
        echo -n "Starting $DESC: "
	$DIR/imucat /dev/ttyUSB0 | $DIR/gulp /var/run/imud >/dev/null 2>&1 &
        echo "OK"
        ;;
  stop)
        echo -n "Stopping $DESC: "
	kill `cat /var/run/imud.pid`
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
