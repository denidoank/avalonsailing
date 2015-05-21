# Setting the hardware clock #
  * `date -s '2011-07-19 14:35:00'`
  * `hwclock --systohc`

# Bootup #
/etc/inittab:
```
# Startup the system
null::sysinit:/bin/mount -t proc proc /proc
null::sysinit:/bin/mount -o remount,rw /
null::sysinit:/bin/mkdir -p /dev/pts
null::sysinit:/bin/mount -a
null::sysinit:/bin/hostname -F /etc/hostname
# now run any rc scripts
::sysinit:/etc/init.d/rcS
```

/etc/init.d/rcS runs /etc/init.d/S`*` in alphabetical order.
It sources S`*`.sh, and runs S`*`