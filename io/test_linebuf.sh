./linebusd /tmp/lbus
N=50
./plug -i /tmp/lbus ./loadtestsend $N &
./plug -i /tmp/lbus ./loadtestsend $N &
./plug -i /tmp/lbus ./loadtestsend $N &
./plug -i /tmp/lbus ./loadtestsend $N &
./plug -o /tmp/lbus ./loadtestrecv &
./plug -o /tmp/lbus ./loadtestrecv &
./plug -o /tmp/lbus ./loadtestrecv &
./plug -o /tmp/lbus ./loadtestrecv &

echo Waiting a bit...
sleep 5

kill `cat /tmp/lbus.pid`
