#! /bin/sh

set -e

DESC="avalon Rudder and Sail control."
DIR=/usr/bin

case "$1" in
  start)
        echo -n "Starting $DESC: "
	$DIR/rudderd /var/run/eposd
        echo "OK"
        ;;
  stop)
        echo -n "Stopping $DESC: "
	kill `cat /var/run/rudderd.pid`
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
