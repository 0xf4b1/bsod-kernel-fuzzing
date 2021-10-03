# syz-bp-cov

Small tool to provide coverage feedback for closed-source targets based on software interrupts for a modified Syzkaller version.

## Building

Since syzkaller maintains the QEMU processes, we decided to modify libvmi to be usable without libvirt. It is expected here to have a modified [libvmi](https://github.com/0xf4b1/libvmi) library installed.

    cd syz-bp-cov
    make

## Usage

It takes the following arguments:

    $ syz-bp-cov <socket> <kernel profile json> <kernel module addr 1> <kernel module breakpoints file 1> <kernel module addr 2> <kernel module breakpoints file 2> ...

In the modified Syzkaller version, it will be called right before the fuzzer inside the worker is executed.

Exemplary Syzkaller configuration files can be found in `syzkaller-configs` directory.

It requires using KVMi-enabled QEMU version with appropriate set arguments for the introspection socket and launching a custom command before the fuzzer starts via the added `command` option like this:

    "command": ["./syz-bp-cov.py", "/tmp/introspector", "<kernel profile>", "<module name 1>", "<breakpoints file 1>", ...]

The script retrieves the kernel module load addresses and wraps syz-bp-cov.

## Limitations

- Fuzzing inside a guest is limited to a single fuzzing process that runs in non-threaded mode
- Due to the breakpoint overhead, the execution speed is low