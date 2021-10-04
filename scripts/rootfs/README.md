# Create minimal fuzzing VM

## Build kernel

Build a Linux kernel with `CONFIG_DEBUG_INFO` enabled. You might consider enabling other options, such as `CONFIG_KASAN` to detect memory bugs although it involves a significant performance overhead.

Generate a kernel profile by using [dwarf2json](https://github.com/tklengyel/dwarf2json/tree/edee6cc49823599c524b6750cb85e3e864aee893)

    $ dwarf2json linux --elf <vmlinux> --system-map <System.map> > profile.json

## Create minimal rootfs

Download alpine minimal root filesystem:

    https://dl-cdn.alpinelinux.org/alpine/v3.13/releases/x86_64/alpine-minirootfs-3.13.5-x86_64.tar.gz

Expand it

    mkdir rootfs
    cd rootfs
    tar xzvf ../alpine-minirootfs-3.13.5-x86_64.tar.gz

Become root

    su

Change owner

    chown -R root:root rootfs

Chroot into it

    chroot rootfs /bin/sh

Create links for needed tools from busybox

    ln -s /bin/busybox /bin/insmod

Set password

    passwd

Install openssh

    apk add openssh

Leave chroot

    exit

Add ssh authorized keys to

    rootfs/root/.ssh/authorized_keys

Additionally, add the following files to the root:

- target kernel modules
- init script
- harness binary (for AFL)
- syz-bp-triage (for Syzkaller)

Repack rootfs

    cd rootfs
    find . -print0 | cpio --null -ov --format=newc | gzip -9 > "../rootfs.cpio.gz"

For booting with QEMU, use the following arguments

    qemu-system-x86_64
        - [...]
        -kernel bzImage
        -initrd rootfs.cpio.gz
        -append "console=ttyS0"