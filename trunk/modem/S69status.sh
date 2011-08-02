#! /bin/sh

set -e

DESC="Avalon SMS status"
DIR=/usr/bin
MODEM_QUEUE=/tmp/iridium

case "$1" in
  start)
        echo -n "Starting $DESC: "
        $DIR/plug -i /var/run/lbus | $DIR/statusd  --no-syslog --logtostderr --task=statusd --timeout=60 --debug --queue=$MODEM_QUEUE --initial_timeout=180 --status_interval=86400 >/dev/null 2>&1 &
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
