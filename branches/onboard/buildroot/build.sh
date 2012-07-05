#!/bin/bash

set -e

BASE_URL=http://buildroot.uclibc.org/downloads/
DATE="$(date +%s)"
BUILDROOT_VER=2012.02
BUILDROOT_TAR="buildroot-$BUILDROOT_VER.tar.bz2"
AVALONSAILING_TAR="avalonsailing-$DATE.tar.gz"
START="$PWD"

function install_packages_if_necessary() {
  while [ ! -z "$1" ] ; do
    dpkg -l "$1" | grep -q ^ii || sudo apt-get install "$1"
    shift
  done
}

if [ "$1" == "-d" ] ; then
  BUILDDIR="$2"
  if [ ! -d "$BUILDDIR" ] ; then
    mkdir -p "$BUILDDIR"
  fi
else
  TMPDIR="$(mktemp -d)"
  BUILDDIR="$TMPDIR"
fi

# Make sure important directory exist
OUTDIR="$START/out"
[ ! -d "$OUTDIR" ] && mkdir -p "$OUTDIR"

BUILDROOTDIR="$BUILDDIR/buildroot-$BUILDROOT_VER/"
export BUILDROOT_DL_DIR="$BUILDROOTDIR/dl"
mkdir -p "$BUILDROOT_DL_DIR"


# install some packages we will need later
install_packages_if_necessary bison flex texinfo unzip gettext

# create an avalonsailing tarball
pushd $START/..
make OUTDIR=$BUILDROOT_DL_DIR DATE=$DATE tarball
popd

pushd $BUILDDIR

if [ ! -f "$BUILDROOT_DL_DIR/$BUILDROOT_TAR" ] ; then
  wget -O "$BUILDROOT_DL_DIR/$BUILDROOT_TAR" \
      "$BASE_URL/$BUILDROOT_TAR"
fi

tar jxf "$BUILDROOT_DL_DIR/$BUILDROOT_TAR"

cp "$START/configs/linux_config" "$BUILDROOTDIR/linux.config"
cp "$START/configs/buildroot_config" "$BUILDROOTDIR/.config"

# start working on buildroot
pushd "$BUILDROOTDIR"

# copy the package definition files into place
tar cpfC - "$START/package/" --exclude .svn --exclude "svn*" . |\
    tar xvpfC - "$BUILDROOTDIR/package"

# modify Config.in
cat <<EOD >> $BUILDROOTDIR/package/Config.in
menu "Sail your boat"
source "package/avalonsailing/Config.in"
endmenu
EOD

# update the name of the tarball
sed -i -e "s/%SOURCE%/$AVALONSAILING_TAR/g" \
    "$BUILDROOTDIR/package/avalonsailing/avalonsailing.mk"

# setup the alternative filesystem layout
tar zxvpf "$START/avalon_skeleton.tar.gz"

# just in case, make sure it has to build avalonsailing again
if [ -d output/build/avalonsailing ] ; then
  rm output/build/avalongsailing/.stamp_*
fi

make
cp output/images/* "$OUTDIR/"
popd

popd
if [ "$BUILDDIR" == "$TMPDIR" ] ; then
  rm -rf "$BUILDDIR"
fi
