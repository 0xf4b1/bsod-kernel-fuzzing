#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/pci/pci.h"
#include "hw/pci/msi.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"

extern bool msi;

typedef struct RegisterValue {
    unsigned int value;
    bool interrupt;
    QLIST_ENTRY(RegisterValue) next;
} RegisterValue;

typedef struct RegisterSequence {
    unsigned long address;
    RegisterValue *current_read;
    QLIST_HEAD(, RegisterValue) reads;
} RegisterSequence;

typedef struct ReplayRegion {
    int id;
    void *memory;
    MemoryRegion mem;
    RegisterSequence **seq;
    unsigned char *type;
} ReplayRegion;

typedef struct ReplayState {
    PCIDevice dev;
    unsigned loglevel;
    bool log;
    bool msi;
    ReplayRegion regions[4];
} ReplayState;

uint64_t read_mem(void *mem, hwaddr addr, unsigned size);
void write_mem(void *mem, hwaddr addr, uint64_t val, unsigned size);
uint64_t region_read(ReplayRegion *region, hwaddr addr, unsigned size, bool log);
void region_write(ReplayRegion *region, hwaddr addr, uint64_t val, unsigned size, bool log);
void request_irq(void *opaque);
void load_seq(ReplayRegion *region, const char *filename);
void load_ram(ReplayRegion *region, const char *memoryfile);
void reset_sequences(ReplayRegion *region);