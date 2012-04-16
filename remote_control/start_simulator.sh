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
$MAKE -C helmsman
$MAKE -C fakeio
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
killall linebusd helmsman fakeboat || true

# Run the bus and the fake boat
./io/linebusd $LBUS
sleep 1

${PLUG} ./fakeio/fakeboat &

# Run the remote_control tool, and configure it.
CONNECT_CMD=$(pwd)"/io/plug ${LBUS}"
echo "remote_control config string: ${CONNECT_CMD}"

if [ "$OSTYPE" == "linux-gnu" ]; then
  # Linux
  ./remote_control/remote_control "${CONNECT_CMD}" &
else
  # Mac, new setup
  ./remote_control/remote_control.app/Contents/MacOS/remote_control "${CONNECT_CMD}" &
fi
 
${PLUG} ./helmsman/helmsman
