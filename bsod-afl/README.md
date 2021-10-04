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

1) Start QEMU and load target module in guest
2) Start bsod-afl on host

```
$ afl-fuzz -i input -o output -t 9999 -- ./bsod-afl --socket /tmp/introspector --json <kernel profile> --input @@ --module <module addr> --breakpoints <breakpoints file> --coverage block
```

3) Start harness in guest

```
$ ./harness
```