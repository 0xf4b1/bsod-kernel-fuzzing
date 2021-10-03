#include "pci-replay.h"

#define REGION_0_SIZE 0x1000000
#define REGION_1_SIZE 0x8000000
#define REGION_3_SIZE 0x2000000
#define IOPORT_SIZE 0x80

#define TYPE_PCI_NVIDIA "nvidia"
#define PCI_NVIDIA(obj) OBJECT_CHECK(ReplayState, (obj), TYPE_PCI_NVIDIA)

static const VMStateDescription vmstate_nvidia = {
    .name = TYPE_PCI_NVIDIA,
    .version_id = 3,
    .minimum_version_id = 3,
    .fields = (VMStateField[]){VMSTATE_PCI_DEVICE(dev, ReplayState), VMSTATE_END_OF_LIST()}};

static void reset_function(DeviceState *dev) {
    ReplayState *state = PCI_NVIDIA(dev);
    for (int i = 0; i < 3; i++) {
        reset_sequences(&state->regions[i]);
    }
}

static uint64_t nvidia_bar0_read(void *ptr, hwaddr addr, unsigned size) {
    if (addr == 0xa00)
        reset_function(ptr);

    return region_read(&PCI_NVIDIA(ptr)->regions[0], addr, size, true);
}

static void nvidia_bar0_write(void *ptr, hwaddr addr, uint64_t val, unsigned size) {
    if (addr == 0x100 && val == 0x80000000)
        request_irq(ptr);

    region_write(&PCI_NVIDIA(ptr)->regions[0], addr, val, size, true);
}

static const MemoryRegionOps pci_nvidia_bar0_ops = {
    .read = nvidia_bar0_read,
    .write = nvidia_bar0_write,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static uint64_t nvidia_bar1_read(void *ptr, hwaddr addr, unsigned size) {
    return region_read(&PCI_NVIDIA(ptr)->regions[1], addr, size, true);
}

static void nvidia_bar1_write(void *ptr, hwaddr addr, uint64_t val, unsigned size) {
    region_write(&PCI_NVIDIA(ptr)->regions[1], addr, val, size, true);
}

static const MemoryRegionOps pci_nvidia_bar1_ops = {
    .read = nvidia_bar1_read,
    .write = nvidia_bar1_write,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static uint64_t nvidia_bar3_read(void *ptr, hwaddr addr, unsigned size) {
    return region_read(&PCI_NVIDIA(ptr)->regions[2], addr, size, true);
}

static void nvidia_bar3_write(void *ptr, hwaddr addr, uint64_t val, unsigned size) {
    region_write(&PCI_NVIDIA(ptr)->regions[2], addr, val, size, true);
}

static const MemoryRegionOps pci_nvidia_bar3_ops = {
    .read = nvidia_bar3_read,
    .write = nvidia_bar3_write,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static uint64_t io_read(void *opaque, hwaddr addr, unsigned size) {
    printf("io_read %lx, %x\n", addr, size);
    return 0x0;
}

static void io_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) {
    printf("io_write %lx, %x, %lx\n", addr, size, val);
}

static const MemoryRegionOps pci_io_ops = {
    .read = io_read,
    .write = io_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 4,
    .impl.min_access_size = 1,
    .impl.max_access_size = 2,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void nvidia_realize(PCIDevice *dev, Error **errp) {
    ReplayState *state = PCI_NVIDIA(dev);
    msi = state->msi;

    // Region 0: Memory at f2000000 (32-bit, non-prefetchable) [size=16M]
    memory_region_init_io(&state->regions[0].mem, OBJECT(dev), &pci_nvidia_bar0_ops, dev, "region0",
                          REGION_0_SIZE);
    pci_register_bar(dev, 0, PCI_BASE_ADDRESS_MEM_TYPE_32, &state->regions[0].mem);
    load_ram(&state->regions[0], "region0.ram");
    load_seq(&state->regions[0], "region0.seq");

    // Region 1: Memory at e8000000 (64-bit, prefetchable) [size=128M]
    memory_region_init_io(&state->regions[1].mem, OBJECT(dev), &pci_nvidia_bar1_ops, dev, "region1",
                          REGION_1_SIZE);
    pci_register_bar(dev, 1, PCI_BASE_ADDRESS_MEM_TYPE_64 | PCI_BASE_ADDRESS_MEM_PREFETCH,
                     &state->regions[1].mem);
    load_ram(&state->regions[1], "region1.ram");
    load_seq(&state->regions[1], "region1.seq");

    // Region 3: Memory at f0000000 (64-bit, prefetchable) [size=32M]
    memory_region_init_io(&state->regions[2].mem, OBJECT(dev), &pci_nvidia_bar3_ops, dev, "region3",
                          REGION_3_SIZE);
    pci_register_bar(dev, 3, PCI_BASE_ADDRESS_MEM_TYPE_64 | PCI_BASE_ADDRESS_MEM_PREFETCH,
                     &state->regions[2].mem);
    load_ram(&state->regions[2], "region3.ram");
    load_seq(&state->regions[2], "region3.seq");

    // Region 5: I/O ports at e000 [size=128]
    memory_region_init_io(&state->regions[3].mem, OBJECT(dev), &pci_io_ops, dev, "region5",
                          IOPORT_SIZE);
    pci_register_bar(dev, 5, PCI_BASE_ADDRESS_SPACE_IO, &state->regions[3].mem);

    pci_set_byte(&dev->config[PCI_INTERRUPT_PIN], 0x1);

    if (msi) {
        int ret = msi_init(dev, 0x68, 1, 1, 0, errp);
        if (ret < 0) {
            printf("MSI setup failed\n");
        }
    }
}

static Property nvidia_dev_properties[] = {
    DEFINE_PROP_BOOL("msi", ReplayState, msi, false),
    DEFINE_PROP_END_OF_LIST(),
};

static void nvidia_pci_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->vendor_id = 0x10de;           // NVIDIA Corporation
    k->device_id = 0x1187;           // GK104 [GeForce GTX 760]
    k->subsystem_vendor_id = 0x1569; // Palit Microsystems Inc.
    k->subsystem_id = 0x1187;        // GK104 [GeForce GTX 760]

    k->realize = nvidia_realize;
    k->class_id = PCI_CLASS_DISPLAY_VGA;
    dc->reset = reset_function;
    dc->hotpluggable = false;
    dc->vmsd = &vmstate_nvidia;
    device_class_set_props(dc, nvidia_dev_properties);
}

static const TypeInfo nvidia_type_info = {
    .name = TYPE_PCI_NVIDIA,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(ReplayState),
    .class_init = nvidia_pci_class_init,
    .interfaces =
        (InterfaceInfo[]){
            {INTERFACE_CONVENTIONAL_PCI_DEVICE},
            {},
        },
};

static void nvidia_register_types(void) { type_register_static(&nvidia_type_info); }

type_init(nvidia_register_types)
