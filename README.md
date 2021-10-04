# bsod-kernel-fuzzing

## Requirements

- [kvm-vmi](kvm-vmi)

    The fuzzing setups rely on the KVM-VMI project that provides introspection capabilities for KVM. It consists of a modified KVM kernel module and QEMU, libkvmi and libvmi.
    To prepare the host, follow the [Setup instructions](https://kvm-vmi.github.io/kvm-vmi/master/setup.html).

- A guest file system image for fuzzing.

    For Linux, you should consider creating a [minimal rootfs](scripts/rootfs).

## [bsod-afl](bsod-afl)

Kernel fuzzing with AFL initially based on [kernel-fuzzer-for-xen-project](https://github.com/intel/kernel-fuzzer-for-xen-project).

## [bsod-syzkaller](bsod-syzkaller)

Modified syzkaller kernel fuzzer with patches for using syz-bp-cov, a small tool that provides coverage feedback via breakpoints intended for fuzzing closed-source targets.

## [bsod-fakedev](bsod-fakedev)

QEMU with pci-replay device and implementation based on a nvidia reference device and scripts to extract pci-replay data out of QEMU's vfio trace data.