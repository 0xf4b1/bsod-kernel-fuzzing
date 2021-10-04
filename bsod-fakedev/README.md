# bsod-fakedev

## Tracing

1. Setup VFIO device passthrough

```
# vfio/bind.sh
```

2. Start QEMU with PCI passthrough and vfio tracing enabled

```
-device vfio-pci,host=01:00.0,x-no-mmap=true,x-no-kvm-intx=true,x-no-kvm-msi=true,x-no-kvm-msix=true -trace events=events
```

The vfio trace data are stored in the `trace.log` file.

3. Extract pci-replay data out of the trace data by using the script

```
$ convert-qemu-trace.py trace.log
$ extract-ram-image.py <region> <start addr>
$ extract-seq-data.py <region> <start addr>
```

The scripts will extract initial RAM images for the memory regions and register value sequences for the memory addresses.

## PCI replay device

With the extracted data from the tracing process in place, start QEMU with the pci-replay device.