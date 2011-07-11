#!/bin/bash

set -e

PASSWORD='$1$Up9w3Mo.$pMuOXqAMX7yMTxFxnhLno0'
BASE_URL=/home/lbedford/www/buildroot
DATE=$(date +%s)
BUILDROOT_VER=2011.05
LINUX_VER=2.6.38.4
BUILDROOT_TAR=buildroot-$BUILDROOT_VER.tar.bz2
LINUX_TAR=linux-$LINUX_VER.tar.bz2
AVALONSAILING_TAR=avalonsailing-$DATE.tar.gz
START=$PWD

function install_packages_if_necessary() {
  while [ ! -z "$1" ] ; do
    dpkg -l "$1" | grep -q ^ii || sudo apt-get install "$1"
    shift
  done
}

if [ "$1" == "-d" ] ; then
  BUILDDIR="$2"
  if [ ! -d "$BUILDDIR" ] ; then
    mkdir -p $BUILDDIR
  fi
else
  TMPDIR=$(mktemp -d)
  BUILDDIR=$TMPDIR
fi
mkdir -p $BUILDDIR/dl
LINUXDIR=$BUILDDIR/linux-$LINUX_VER/
BUILDROOTDIR=$BUILDDIR/buildroot-$BUILDROOT_VER/
export BUILDROOT_DL_DIR=$BUILDROOTDIR/dl

OUTDIR="$PWD/out"
[ ! -d "$OUTDIR" ] && mkdir -p "$OUTDIR"

# install some packages we will need later
install_packages_if_necessary bison flex dropbear texinfo unzip gettext

# create an avalonsailing tarball
pushd $START/..
mkdir -p $BUILDROOT_DL_DIR
make OUTDIR=$BUILDROOT_DL_DIR DATE=$DATE tarball
popd

pushd $BUILDDIR
tar jxf $BASE_URL/$BUILDROOT_TAR
tar jxf $BASE_URL/$LINUX_TAR

cp $START/configs/linux_config $LINUXDIR/.config
cp $START/configs/buildroot_config $BUILDROOTDIR/.config

# start working on buildroot
pushd $BUILDROOTDIR

# copy the package definition files into place
tar cpfC - $START/package/ --exclude .svn --exclude "svn*" . |\
    tar xvpfC - $BUILDROOTDIR/package

# update the name of the tarball
sed -i -e "s/%SOURCE%/$AVALONSAILING_TAR/g" \
    $BUILDROOTDIR/package/avalonsailing/avalonsailing.mk

# setup the SSH server config
DROPBEARDIR=package/customize/source/etc/dropbear
if [ ! -d "$DROPBEARDIR" ] ; then
  mkdir -p $DROPBEARDIR
  dropbearkey -t rsa -f $DROPBEARDIR/dropbear_rsa_host_key
  dropbearkey -t dss -f $DROPBEARDIR/dropbear_dss_host_key
fi

# setup the network
cat >>fs/skeleton/etc/network/interfaces <<EOD
auto eth0
iface eth0 inet static
  address 192.168.0.2
  netmask 255.255.255.0
  gateway 192.168.0.1
EOD

# set passwords
sed -i -e "s/root::/root:$PASSWORD:/" -e "s/default::/default:$PASSWORD:/" \
    fs/skeleton/etc/shadow

# just in case, make sure it has to build avalonsailing again
if [ -d output/build/avalonsailing ] ; then
  rm output/build/avalongsailing/.stamp_*
fi

make
cp output/images/* $OUTDIR/
popd

pushd $LINUXDIR
make ARCH=i386 oldconfig
make ARCH=i386 -j12 bzImage
cp arch/x86/boot/bzImage $OUTDIR/vmlinuz
popd

popd
if [ "$BUILDDIR" == "$TMPDIR" ] ; then
  rm -rf $BUILDDIR
fi
