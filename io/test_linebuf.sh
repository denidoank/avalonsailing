./linebusd /tmp/lbus
./plug -i /tmp/lbus ./loadtestsend 10 &
./plug -i /tmp/lbus ./loadtestsend 10 &
./plug -i /tmp/lbus ./loadtestsend 10 &
./plug -i /tmp/lbus ./loadtestsend 10 &
./plug -o /tmp/lbus ./loadtestrecv &
./plug -o /tmp/lbus ./loadtestrecv &
./plug -o /tmp/lbus ./loadtestrecv &
./plug -o /tmp/lbus ./loadtestrecv &

echo Waiting a bit...
sleep 5

kill `cat /tmp/lbus.pid`
