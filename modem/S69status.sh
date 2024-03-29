#! /bin/sh

set -e

DESC="Avalon SMS status"
DIR=/usr/bin
MODEM_QUEUE=/tmp/modem

case "$1" in
  start)
        echo -n "Starting $DESC: "
        $DIR/plug -o /var/run/lbus | $DIR/statusd --queue=$MODEM_QUEUE --initial_timeout=180 --status_interval=86400  2>/dev/null | $DIR/plug -i /var/run/lbus &
        echo "OK"
        ;;
  stop)
        echo -n "Stopping $DESC: "
	kill `cat /var/run/status.pid`
	killall statusd
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
