#include "fuzz.h"
#include "private.h"

extern csh cs_handle;

static addr_t next_cf_vaddr;
static addr_t next_cf_paddr;
static uint8_t cf_backup;
static bool reinject = false;

static void breakpoint_next_cf(vmi_instance_t vmi) {
    if (VMI_SUCCESS == vmi_read_pa(vmi, next_cf_paddr, 1, &cf_backup, NULL) &&
        VMI_SUCCESS == vmi_write_pa(vmi, next_cf_paddr, 1, &cc, NULL)) {
        if (debug)
            printf("[TRACER] Next CF: 0x%lx -> 0x%lx\n", next_cf_vaddr, next_cf_paddr);
    }
}

static inline bool is_cf(unsigned int id) {
    switch (id) {
    case X86_INS_JA:
    case X86_INS_JAE:
    case X86_INS_JBE:
    case X86_INS_JB:
    case X86_INS_JCXZ:
    case X86_INS_JECXZ:
    case X86_INS_JE:
    case X86_INS_JGE:
    case X86_INS_JG:
    case X86_INS_JLE:
    case X86_INS_JL:
    case X86_INS_JMP:
    case X86_INS_LJMP:
    case X86_INS_JNE:
    case X86_INS_JNO:
    case X86_INS_JNP:
    case X86_INS_JNS:
    case X86_INS_JO:
    case X86_INS_JP:
    case X86_INS_JRCXZ:
    case X86_INS_JS:
    case X86_INS_CALL:
    case X86_INS_RET:
    case X86_INS_RETF:
    case X86_INS_RETFQ:
    case X86_INS_SYSCALL:
    case X86_INS_SYSRET:
        return true;
    default:
        break;
    }

    return false;
}

#define TRACER_CF_SEARCH_LIMIT 100u

static bool next_cf_insn(vmi_instance_t vmi, addr_t dtb, addr_t start) {
    cs_insn *insn;
    size_t count;

    size_t read, search = 0;
    unsigned char buff[15];
    bool found = false;
    access_context_t ctx = {.translate_mechanism = VMI_TM_PROCESS_DTB, .dtb = dtb, .addr = start};

    while (!found && search < TRACER_CF_SEARCH_LIMIT) {
        memset(buff, 0, 15);

        if (VMI_FAILURE == vmi_read(vmi, &ctx, 15, buff, &read) && !read) {
            if (debug)
                printf("Failed to grab memory from 0x%lx with PT 0x%lx\n", start, dtb);
            goto done;
        }

        count = cs_disasm(cs_handle, buff, read, ctx.addr, 0, &insn);
        if (!count) {
            if (debug)
                printf("No instruction was found at 0x%lx with PT 0x%lx\n", ctx.addr, dtb);
            goto done;
        }

        size_t j;
        for (j = 0; j < count; j++) {

            ctx.addr = insn[j].address + insn[j].size;

            if (debug)
                printf("Next instruction @ 0x%lx: %s, size %i!\n", insn[j].address,
                       insn[j].mnemonic, insn[j].size);

            if (is_cf(insn[j].id)) {
                next_cf_vaddr = insn[j].address;
                if (VMI_FAILURE == vmi_pagetable_lookup(vmi, dtb, next_cf_vaddr, &next_cf_paddr)) {
                    if (debug)
                        printf("Failed to lookup next instruction PA for 0x%lx with PT 0x%lx\n",
                               next_cf_vaddr, dtb);
                    break;
                }

                found = true;

                if (debug)
                    printf("Found next control flow instruction @ 0x%lx: %s!\n", next_cf_vaddr,
                           insn[j].mnemonic);
                break;
            }
        }
        cs_free(insn, count);
    }

    if (!found && debug)
        printf("Didn't find a control flow instruction starting from 0x%lx with a search limit %u! "
               "Counter: %lu\n",
               start, TRACER_CF_SEARCH_LIMIT, tracer_counter);

done:
    return found;
}

event_response_t handle_event_dynamic(vmi_instance_t vmi, vmi_event_t *event) {
    afl_instrument_location(event->x86_regs->rip - module_start);

    if (VMI_EVENT_SINGLESTEP == event->type) {
        if (reinject) {
            vmi_write_pa(vmi, next_cf_paddr, 1, &cc, NULL);
            reinject = false;
        } else if (next_cf_insn(vmi, event->x86_regs->cr3, event->x86_regs->rip))
            breakpoint_next_cf(vmi);

        return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP;
    }

    // Reached target address for fuzzing
    if (target_offset && event->x86_regs->rip == module_start + target_offset) {
        if (debug)
            printf("VM reached the target address.\n");

        access_context_t ctx = {.translate_mechanism = VMI_TM_PROCESS_DTB,
                                .dtb = event->x86_regs->cr3,
                                .addr = event->x86_regs->rip};
        vmi_write_8(vmi, &ctx, &target_byte);

        stop(false);
        return 0;
    }

    /*
     * Let's allow the control-flow instruction to execute
     * and catch where it continues using MTF singlestep.
     */
    vmi_write_pa(vmi, next_cf_paddr, 1, &cf_backup, NULL);

    if (trace_pid && current_pid != harness_pid)
        reinject = true;

    if (limit == ~0ul || tracer_counter < limit)
        return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP;

    if (debug)
        printf("Hit the tracer limit: %lu\n", tracer_counter);

    return 0;
}

bool start_trace(vmi_instance_t vmi, addr_t address) {
    if (debug)
        printf("Starting trace from 0x%lx.\n", address);

    next_cf_vaddr = 0;
    next_cf_paddr = 0;
    tracer_counter = 0;

    registers_t regs = {0};
    vmi_get_vcpuregs(vmi, &regs, 0);

    if (!next_cf_insn(vmi, regs.x86.cr3, address)) {
        if (debug)
            printf("Failed starting trace from 0x%lx\n", address);
        return false;
    }

    breakpoint_next_cf(vmi);
    return true;
}