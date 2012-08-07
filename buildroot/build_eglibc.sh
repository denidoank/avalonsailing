#!/bin/bash

set -e

BUILDROOT_URL="http://buildroot.uclibc.org/downloads/"
BUILDROOT_VER=2012.05
BUILDROOT_TAR="buildroot-$BUILDROOT_VER.tar.bz2"

LINUX_KERNEL_URL="http://www.kernel.org/pub/linux/kernel/v3.0"
LINUX_KERNEL_VER="-3.4.7"
LINUX_KERNEL_TAR="linux$LINUX_KERNEL_VER.tar.bz2"
LINUX_KERNEL_URL_GEODE=$LINUX_KERNEL_URL/$LINUX_KERNEL_TAR
LINUX_KERNEL_URL_DREAMPLUG=$LINUX_KERNEL_URL_GEODE
LINUX_KERNEL_URL_RPI="http://www.lbedford.org/linux-rpi.tar.bz2"

X_TOOLS_URL="http://www.lbedford.org/x-tools/"
X_TOOLS_ARM="arm-unknown-linux-gnueabi"
X_TOOLS_X86="i686-nptl-linux-gnu"

DATE="$(date +%s)"
AVALONSAILING_TAR="avalonsailing-$DATE.tar.gz"

START="$PWD"
TARGET="geode"

# TODO: download the cross compilers and use them.

function install_packages_if_necessary() {
  while [ ! -z "$1" ] ; do
    dpkg -l "$1" | grep -q ^ii || sudo apt-get install "$1"
    shift
  done
}

function install_x_tools() {
  X_TOOLS_TAR=${X_TOOLS_ARM}
  if [ "$1" == "x86" ] ; then
    X_TOOLS_TAR=${X_TOOLS_X86}
  fi
  if [ ! -d $X_TOOLS_TAR ] ; then
    wget -O - ${X_TOOLS_URL}/${X_TOOLS_TAR}.tar.bz2 | tar jxf -
    chmod -R u+w $X_TOOLS_TAR
  fi
}

while [ $# -gt 1 ] ; do
  if [ "$1" == "-d" ] ; then
    BUILDDIR="$2"
    if [ ! -d "$BUILDDIR" ] ; then
      mkdir -p "$BUILDDIR"
    fi
    shift 2
  elif [ "$1" == "-t" ] ; then
    TARGET="$2"
    shift 2
 fi
done
  
if [ -z "$BUILDDIR" ] ; then
  TMPDIR="$(mktemp -d)"
  BUILDDIR="$TMPDIR"
fi

if [ "${TARGET,,}" == "geode" ] ; then
  ARCH=x86
else
  ARCH=arm
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

install_x_tools $ARCH

if [ ! -f "$BUILDROOT_DL_DIR/$BUILDROOT_TAR" ] ; then
  wget -O "$BUILDROOT_DL_DIR/$BUILDROOT_TAR" \
      "$BUILDROOT_URL/$BUILDROOT_TAR"
fi

tar jxf "$BUILDROOT_DL_DIR/$BUILDROOT_TAR"

cp "$START/configs/buildroot_config_$ARCH" "$BUILDROOTDIR/.config"

X_TOOLS_DIR=$X_TOOLS_ARM
if [ "$ARCH" == "x86" ] ; then
  X_TOOLS_DIR=$X_TOOLS_X86
fi

echo "BR2_TOOLCHAIN_EXTERNAL_PATH=\"$BUILDDIR/$X_TOOLS_DIR\"" >> "$BUILDROOTDIR/.config"
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

KERNEL_URL_VAR="LINUX_KERNEL_URL_${TARGET^^}"

# Bleugh, need to find a better way of doing this.
if [ "${TARGET,,}" = "rpi" ] ; then
  unset LINUX_KERNEL_VER
fi

if [ ! -f "$BUILDROOT_DL_DIR/$LINUX_KERNEL_TAR" ] ; then
  wget -O "$BUILDROOT_DL_DIR/$LINUX_KERNEL_TAR" \
      ${!KERNEL_URL_VAR}
fi

tar jxf "$BUILDROOT_DL_DIR/$LINUX_KERNEL_TAR"

# now build a linux kernel
LINUXROOTDIR="$BUILDDIR/linux$LINUX_KERNEL_VER/"

pushd $LINUXROOTDIR
cp "$START/configs/linux_config_$TARGET" "$LINUXROOTDIR/.config"
sed -i -e "/CONFIG_INITRAMFS_SOURCE=/d" "$LINUXROOTDIR/.config"
echo "CONFIG_INITRAMFS_SOURCE=\"$OUTDIR/rootfs.cpio.bz2\"" >> "$LINUXROOTDIR/.config"
export PATH=$BUILDDIR/$X_TOOLS_DIR/bin:$PATH
make ARCH=$ARCH -j12
cp arch/$ARCH/boot/*zImage "$OUTDIR/"

popd
if [ "$BUILDDIR" == "$TMPDIR" ] ; then
  rm -rf "$BUILDDIR"
fi
