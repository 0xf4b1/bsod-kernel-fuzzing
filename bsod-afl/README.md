# bsod-afl

Requires KVM-VMI setup for VM introspection.

## Building

```
$ mkdir build && cd build
$ cmake ..
```

## Prepare target

1) Extract addresses of control flow instructions from your target with `extract-breakpoints.py`.
2) Implement a harness for the target according to `harness.c` and copy it to the guest VM.

## Fuzzing

1) Start QEMU with introspection enabled by adding the following parameters.

```
-chardev socket,path=/tmp/introspector,id=chardev0,reconnect=10
-object secret,id=key0,data=some
-object introspection,id=kvmi,chardev=chardev0,key=key0"
```

2) When the VM is booted, load the target module inside the guest and retrieve the load address.

```
# insmod module.ko
# cat /proc/modules
```

3) Start bsod-afl on host.

```
$ afl-fuzz -i input -o output -t 9999 -- ./bsod-afl --socket /tmp/introspector --json <kernel profile> --input @@ --module <module addr> --breakpoints <breakpoints file> --coverage block
```

4) Start harness in guest.

```
$ ./harness
```