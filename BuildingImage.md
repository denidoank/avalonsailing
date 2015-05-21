# Introduction #

This document will go through the steps necessary to build and deploy
an image of the avalon sailing system.

# Details #

## Useful features of the avalon sailing build system: ##
  * `make tarball`
    * Creates a tar.gz file of the current source
  * `make image [ IMAGE_DIR=/tmp/avalon ]`
    * runs buildroot/build.sh (optionally persisting the output in $(IMAGE\_DIR)

## Steps to build with uclibc locally on ubuntu machines ##
  * `buildroot/build.sh -d /usr/local/google/buildroot`
  * `export PATH=/usr/local/google/buildroot/buildroot-2011.05/output/host/usr/bin/:$PATH`
  * `make CC=i586-linux-gcc CXX=i586-linux-g++ LD=i586-linux-ld AR=i586-linux-ar`

## Steps to build an image: ##
  * checkout the source code (or svn up if you already have it)
  * [Prepare](#Prepare_CF.md) a CF card
  * `make image`
  * mount CF somewhere
    * `mount /dev/sdb1 /mnt`
  * cp buildroot/out/vmlinuz /mnt
  * cp buildroot/out/rootfs.ext2.gz /mnt/initrd.img
  * `sync; umount /mnt`

## Prepare CF ##
  * find out what device it appeared as (say /dev/sdb)
  * `apt-get install syslinux mbr`
  * make a VFAT filesystem on it
    * `fdisk /dev/sdb`
    * `mkfs.vfat -F32 /dev/sdb1`
  * `install-mbr /dev/sdb`
  * `syslinux /dev/sdb`
  * find a syslinux.cfg for it (if we haven't made one already)

## Update CF in machine ##
  * `mkdir /cfcard`
  * `mount /dev/sda1 /cfcard`
  * `cp /path/to/bzImage /cfcard/vmlinuz`
  * `cp /path/to/rootfs.ext2.gz /cfcard/initrd.img`
  * `umount /cfcard`
  * (not necessary) `sync`

## Quick rebuild of persistent buildroot ##
  * `BUILDROOT=/tmp/avalon`
  * `make image IMAGE_DIR=$BUILDROOT`
Now make modifications to your svn source, and run the last command when you want to build
  * `cp buildroot/out/rootfs.ext2.gz /path/to/where/you/want/it`

## Full Rebuild of persistent buildroot ##
This is more complicated.

  * `BUILDROOT=/tmp/avalon/buildroot-2011.05`
  * `cd $BUILDROOT`
  * `cd output/build/avalonsailing`
  * make whatever changes you want
  * invalidate the build stamps
    * `rm -rf .stamps_*`
  * `cd $BUILDROOT`
  * `make`
  * `cp output/images/rootfs.ext2.gz /path/to/where/you/want/it`