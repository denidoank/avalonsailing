# Author: Julien Pilet <jpilet@google.com>
#
# Shell script to (re)start basic simulation.

set -e 

MAKE="make -j4"
LBUS="/tmp/lbus"
PLUG="./io/plug ${LBUS}"

# Compile what we need.
$MAKE -C common
$MAKE -C io
$MAKE -C fakeio
$MAKE -C helmsman
pushd remote_control
if which qmake > /dev/null; then
  qmake
  make -j 4
else
  echo "Please run: sudo apt-get install libqt4-dev"
  exit -1
fi
popd

# killing linebusd should kill everything.
killall linebusd fakeimu fakerudderd fakewind || true

# Run the bus and the fake boat
./io/linebusd $LBUS
sleep 1

${PLUG} ./fakeio/fakeboat &

sleep 1

${PLUG} ./helmsman/helmsman &

sleep 1

# Run the remote_control tool, and configure it.
CONNECT_CMD=$(pwd)"/io/plug ${LBUS}"
echo "remote_control config string: ${CONNECT_CMD}"
./remote_control/remote_control "${CONNECT_CMD}"
