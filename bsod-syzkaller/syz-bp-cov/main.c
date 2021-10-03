#include <assert.h>
#include <stdlib.h>
// clang-format off
#include <libvmi/libvmi.h>
#include <libvmi/events.h>
// clang-format on

#include "breakpoint.h"

static vmi_instance_t vmi;
static vmi_event_t singlestep_event, cc_event;

static unsigned long *modules;
static struct table *breakpoints;
static struct breakpoint *current_bp;

static uint8_t cc = 0xCC;
static uint8_t nop = 0x90;

static addr_t coverage;
static size_t cover_size = 0;
static bool triage = false;

static void report_coverage(unsigned long pc) {
    unsigned long size;
    if (VMI_SUCCESS != vmi_read_pa(vmi, coverage, sizeof(unsigned long), &size, NULL)) {
        printf("Can not get current coverage size\n");
        return;
    }

    size++;
#ifdef DEBUG
    printf("pc: 0x%lx, size: %u\n", pc, size);
#endif

    if (size >= cover_size) {
        printf("Max cover size reached!\n");
        return;
    }

    if (VMI_SUCCESS != vmi_write_pa(vmi, coverage, sizeof(unsigned long), &size, NULL)) {
        printf("Can not write new coverage size\n");
        return;
    }

    if (VMI_SUCCESS != vmi_write_pa(vmi, coverage + size * sizeof(unsigned long),
                                    sizeof(unsigned long), &pc, NULL)) {
        printf("Can not write new coverage\n");
        return;
    }

    vmi_pagecache_flush(vmi);
}

static void setup_breakpoints(vmi_instance_t vmi) {
    for (int pos = 0; pos < breakpoints->size; pos++) {
        struct node *node = breakpoints->nodes[pos];
        while (node) {
            if (!node->breakpoint->taken)
                assert(VMI_SUCCESS ==
                       vmi_write_va(vmi, node->breakpoint->address, 0, 1, &cc, NULL));
            node = node->next;
        }
    }
}

static event_response_t tracer_cb(vmi_instance_t vmi, vmi_event_t *event) {
#ifdef DEBUG
    printf("[TRACER] %s: 0x%lx\n", VMI_EVENT_SINGLESTEP == event->type ? "SINGESTEP" : "INTERRUPT",
           event->x86_regs->rip);
#endif
    event->interrupt_event.reinject = 0;

    if ((event->x86_regs->rax >> 8) == 0x13371337133713) {
        uint8_t cmd = event->x86_regs->rax & 0xff;
        switch (cmd) {
        case 0x41: {
            unsigned long cover_va = event->x86_regs->rbx;
#ifdef DEBUG
            printf("hypercall open_cover cover_va: %lx\n", cover_va);
#endif
            if (cover_va == 0x10000) {
                assert(VMI_SUCCESS ==
                       vmi_pagetable_lookup(vmi, event->x86_regs->cr3, cover_va, &coverage));
                cover_size = event->x86_regs->rcx;
            }
            break;
        }
        case 0x42:
#ifdef DEBUG
            printf("hypercall enable_triage\n");
#endif
            if (!triage) {
                setup_breakpoints(vmi);
                triage = true;
            }
            break;
        case 0x43:
#ifdef DEBUG
            printf("hypercall disable_triage\n");
#endif
            triage = false;
            break;
        }

        event->x86_regs->rip += 1;
        return VMI_EVENT_RESPONSE_SET_REGISTERS;
    }

    current_bp = get_breakpoint(breakpoints, event->x86_regs->rip);
    assert(current_bp);

    if (coverage)
        report_coverage(event->x86_regs->rip - modules[current_bp->module_id] + 0xffffffff80000000 +
                        current_bp->module_id * 0x10000000);
    else
        current_bp->taken = true;

    if (event->x86_regs->rip == current_bp->address) {
        assert(VMI_SUCCESS ==
               vmi_write_va(vmi, current_bp->address, 0, 1, &current_bp->cf_backup, NULL));
        if (!current_bp->taken) {
            assert(VMI_SUCCESS == vmi_write_va(vmi, current_bp->taken_addr, 0, 1, &cc, NULL));
            assert(VMI_SUCCESS == vmi_write_va(vmi, current_bp->not_taken_addr, 0, 1, &cc, NULL));
        }
        return 0;
    }

    if (event->x86_regs->rip == current_bp->taken_addr)
        assert(VMI_SUCCESS ==
               vmi_write_va(vmi, current_bp->taken_addr, 0, 1, &current_bp->cf_backup_taken, NULL));
    else
        assert(VMI_SUCCESS == vmi_write_va(vmi, current_bp->not_taken_addr, 0, 1,
                                           &current_bp->cf_backup_not_taken, NULL));

    if (triage)
        assert(VMI_SUCCESS == vmi_write_va(vmi, current_bp->address, 0, 1, &cc, NULL));

    return 0;
}

static bool setup_trace(vmi_instance_t vmi, int argc, char *argv[]) {
    printf("Setup trace\n");

    SETUP_INTERRUPT_EVENT(&cc_event, tracer_cb);

    if (VMI_FAILURE == vmi_register_event(vmi, &cc_event))
        return false;

    breakpoints = create_table(0x1000);
    modules = calloc(argc - 3, sizeof(unsigned long));

    char buf[1024];
    for (int i = 0; i < (argc - 3) / 2; i++) {
        modules[i] = strtoul(argv[i * 2 + 3], NULL, 0);

        FILE *fp = fopen(argv[i * 2 + 3 + 1], "r");
        assert(fp);

        while (fgets(buf, 1024, fp)) {
            char *line = strtok(buf, "\n");

            unsigned long address = modules[i] + strtoul(strtok(line, ","), NULL, 16);
            unsigned long taken_addr = modules[i] + strtoul(strtok(NULL, ","), NULL, 16);
            unsigned long not_taken_addr = modules[i] + strtoul(strtok(NULL, ","), NULL, 16);

            unsigned char cf_backup, cf_backup_taken, cf_backup_not_taken;

            assert(VMI_SUCCESS == vmi_read_va(vmi, address, 0, 1, &cf_backup, NULL));
            assert(VMI_SUCCESS == vmi_read_va(vmi, taken_addr, 0, 1, &cf_backup_taken, NULL));
            assert(VMI_SUCCESS ==
                   vmi_read_va(vmi, not_taken_addr, 0, 1, &cf_backup_not_taken, NULL));

            insert_breakpoint(breakpoints, i, address, taken_addr, not_taken_addr, cf_backup,
                              cf_backup_taken, cf_backup_not_taken);
        }
    }
    setup_breakpoints(vmi);
    printf("Setup trace finished\n");
    return true;
}

static bool setup_vmi(vmi_instance_t *vmi, char *socket, char *json) {
    vmi_init_data_t *init_data = malloc(sizeof(vmi_init_data_t) + sizeof(vmi_init_data_entry_t));
    init_data->count = 1;
    init_data->entry[0].type = VMI_INIT_DATA_KVMI_SOCKET;
    init_data->entry[0].data = strdup(socket);

    if (VMI_FAILURE ==
        vmi_init(vmi, VMI_KVM, NULL, VMI_INIT_EVENTS | VMI_INIT_DOMAINNAME, init_data, NULL))
        return false;

    if (VMI_OS_UNKNOWN == vmi_init_os(*vmi, VMI_CONFIG_JSON_PATH, json, NULL)) {
        vmi_destroy(*vmi);
        return false;
    }

    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: %s <socket> <kernel json profile> <kernel module 1 address> <kernel module "
               "1 breakpoints file> ...\n",
               argv[0]);
        return -1;
    }

    if (!setup_vmi(&vmi, argv[1], argv[2])) {
        fprintf(stderr, "Unable to start VMI on domain\n");
        return -1;
    }

    setup_trace(vmi, argc, argv);

    while (true) {
        if (vmi_events_listen(vmi, 500) == VMI_FAILURE) {
            fprintf(stderr, "Error in vmi_events_listen!\n");
            break;
        }
    }
}
