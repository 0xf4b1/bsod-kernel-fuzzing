#ifndef SINK_H
#define SINK_H

/*
 * List all sink points here. When the kernel executes any of these functions
 * we will report a crash to AFL and stop the fuzzer.
 */
enum sink_enum { OOPS_BEGIN, PANIC, KASAN_REPORT, __SINK_MAX };

/* Now define what symbol each enum entry corresponds to in the debug json */
const char *sinks[] = {
    [PANIC] = "panic",

    /*
     * We can define as many sink points as we want. These sink points don't have
     * to be strictly functions that handle "crash" situations. We can define any
     * code location as a sink point that we would want to know about if it is reached
     * during fuzzing. For example the testmodule triggering a NULL-deref doesn't crash
     * the kernel, it simply causes an "oops" message to be printed to the kernel logs.
     * However, if there is an input that causes something like that then it warrants
     * being recorded.
     *
     * So in essence we can define the sink points as anything of interest that we would
     * want AFL to record if its reached.
     */
    [OOPS_BEGIN] = "oops_begin",
    [KASAN_REPORT] = "kasan_report",
};

addr_t sink_vaddr[__SINK_MAX] = {
    [0 ... __SINK_MAX - 1] = 0,

    /*
     * You can manually define each sink's virtual address here. For example:
    [PAGE_FAULT] = 0xffffffdeadbeef,
     */
};

addr_t sink_paddr[__SINK_MAX] = {
    [0 ... __SINK_MAX - 1] = 0,

    /*
     * You can manually define each sink's physical address here. For example:
    [PAGE_FAULT] = 0xdeadbeef,
     */
};

uint8_t sink_backup[__SINK_MAX] = {0};

#endif
