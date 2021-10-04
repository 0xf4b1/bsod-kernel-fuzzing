# bsod-syzkaller

Requires KVM-VMI setup for VM introspection.

## Building

1) Build syzkaller

```
$ cd syzkaller
$ make generate
$ make
```

2) Build syz-bp-cov

```
$ cd syz-bp-cov
$ make
```

## Prepare target

1) Extract addresses of control flow instructions from your target with `extract-breakpoints.py`.
2) Create [syscall descriptions](https://github.com/google/syzkaller/blob/master/docs/syscall_descriptions.md) for your target.

## Fuzzing

1) Create a syzkaller [configuration](https://github.com/google/syzkaller/blob/master/docs/configuration.md) file similarly to `sample.cfg`.
Make sure to add the following line for syz-bp-cov to specify target modules and related breakpoint files.

```
"command": ["./syz-bp-cov.py", "/tmp/introspector{{INDEX}}", "<kernel profile>", "<first module name>", "<first module breakpoints file>", "<second module name>", "<second module breakpoints file>", ...]
```

2) Start syzkaller

```
$ syz-manager -config <config file>
```