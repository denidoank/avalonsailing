#/bin/sh

D=`mktemp -d /tmp/loadtest.XXXXXX`
N=100

echo dir = $D

./linebusd $D/lbus

./plug -i $D/lbus ./loadtestsend $N &
./plug -i $D/lbus ./loadtestsend $N &
./plug -i $D/lbus ./loadtestsend $N &
./plug -i $D/lbus ./loadtestsend $N &
./plug -o $D/lbus ./loadtestrecv 2> $D/1.out  &
./plug -o $D/lbus ./loadtestrecv 2> $D/2.out  &
./plug -o $D/lbus | ./loadtestrecv 2> $D/3.out  &
./plug -o $D/lbus | ./loadtestrecv 2> $D/4.out  &

echo Waiting a bit...
sleep 10

kill `cat $D/lbus.pid`

cat $D/?.out
