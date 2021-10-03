#include "pci-replay.h"

bool msi;

uint64_t read_mem(void *mem, hwaddr addr, unsigned size) {
    switch (size) {
    case 1:
        return *(unsigned char *)(mem + addr);
        break;
    case 2:
        return *(unsigned short *)(mem + addr);
        break;
    case 4:
        return *(unsigned int *)(mem + addr);
        break;
    case 8:
        return *(unsigned long *)(mem + addr);
        break;
    default:
        printf("ERROR read size: %x\n", size);
        return 0x0;
    }
}

void write_mem(void *mem, hwaddr addr, uint64_t val, unsigned size) {
    switch (size) {
    case 1:
        *(unsigned char *)(mem + addr) = val;
        break;
    case 2:
        *(unsigned short *)(mem + addr) = val;
        break;
    case 4:
        *(unsigned int *)(mem + addr) = val;
        break;
    case 8:
        *(unsigned long *)(mem + addr) = val;
        break;
    default:
        printf("ERROR write size: %x\n", size);
    }
}

uint64_t region_read(ReplayRegion *region, hwaddr addr, unsigned size, bool log) {
    unsigned char type = region->type[addr >> 2];
    if (type >= 2) {
        RegisterSequence *seq = region->seq[type - 2];
        if (seq->current_read != NULL) {
            write_mem(region->memory, addr, seq->current_read->value, size);

            if (seq->current_read->interrupt)
                request_irq(region->mem.opaque);

            seq->current_read = QLIST_NEXT(seq->current_read, next);
        }
    }

    return read_mem(region->memory, addr, size);
}

void region_write(ReplayRegion *region, hwaddr addr, uint64_t val, unsigned size, bool log) {
    if (region->type[addr >> 2] == 1)
        return;

    write_mem(region->memory, addr, val, size);
}

void request_irq(void *dev) {
    printf("IRQ: %lu\n", qemu_clock_get_us(QEMU_CLOCK_VIRTUAL));

    if (msi)
        msi_notify(dev, 0);
    else {
        pci_irq_assert(dev);
        pci_irq_deassert(dev);
    }
}

void load_seq(ReplayRegion *region, const char *filename) {
    printf("Loading seq data %s\n", filename);

    FILE *fp = fopen(filename, "r");
    region->type = calloc(region->mem.size >> 2, 1);
    region->seq = calloc(254, sizeof(RegisterSequence *));

    unsigned char current_index = 0;
    bool next_int = false;
    char buf[1024];

    while (fgets(buf, 1024, fp)) {

        // remove tailing newline
        char *line = strtok(buf, "\n");

        // operation is first comma-seperated value
        char *op = strtok(line, ",");

        if (!strcmp(op, "W"))
            continue;

        // parse interrupts
        if (!strcmp(op, "INT")) {
            // next event will trigger an interrupt
            next_int = true;
            continue;
        }

        // address is next value
        unsigned long address = strtoul(strtok(NULL, ","), NULL, 16);

        if (!strcmp(op, "RO")) {
            region->type[address >> 2] = 1;
            continue;
        }

        // followed by value
        unsigned long value = strtoul(strtok(NULL, ","), NULL, 16);

        RegisterValue *item = malloc(sizeof(RegisterValue));
        item->value = value;
        item->interrupt = false;

        if (next_int) {
            item->interrupt = true;
            next_int = false;
        }

        RegisterSequence *seq;
        unsigned char index = region->type[address >> 2];

        if (index == 0) {
            seq = malloc(sizeof(RegisterSequence));
            seq->address = address;
            seq->current_read = item;
            QLIST_INIT(&seq->reads);
            QLIST_INSERT_HEAD(&seq->reads, item, next);

            region->type[address >> 2] = current_index + 2;
            region->seq[current_index++] = seq;
            continue;
        }

        seq = region->seq[index - 2];
        QLIST_INSERT_AFTER(seq->current_read, item, next);
        seq->current_read = item;
    }

    reset_sequences(region);
}

void load_ram(ReplayRegion *region, const char *memoryfile) {
    printf("Loading ram image %s\n", memoryfile);
    int fd = open(memoryfile, O_RDONLY);
    region->memory = mmap(NULL, region->mem.size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
}

void reset_sequences(ReplayRegion *region) {
    for (unsigned int i = 0; i < 254; i++) {
        if (region->seq[i] != NULL)
            region->seq[i]->current_read = QLIST_FIRST(&region->seq[i]->reads);
    }
}