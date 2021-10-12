#include "fuzz.h"
#include "private.h"
#include "tracer_dynamic.h"

static bool fuzz_loop = false;

static bool get_input(void) {
    if (!input_limit)
        return false;

    if (debug)
        printf("Get %lu bytes of input from %s\n", input_limit, input_path);

    input_file = fopen(input_path, "r");
    if (!input_file)
        return false;

    if (!input) {
        fclose(input_file);
        input_file = NULL;
        return false;
    }

    memset(input, 0, input_limit);
    if (!(input_size = fread(input, 1, input_limit, input_file)))
        return false;

    fclose(input_file);
    input_file = NULL;

    if (debug)
        printf("Got input size %lu\n", input_size);

    return true;
}

static bool inject_input() {
    if (!input || !input_size)
        return false;

    if (debug)
        printf("Writing %lu bytes of input to 0x%lx\n", input_size, address_pa);

    return VMI_SUCCESS == vmi_write_pa(vmi, address_pa, input_limit, input, NULL);
}

bool fuzz() {
    if (fuzz_loop)
        stop(false);

    if (debug)
        printf("Starting fuzz loop\n");

    if (afl) {
        afl_rewind(module_start + start_offset);
        afl_wait();
    }

    if (!get_input())
        return false;

    if (!inject_input()) {
        printf("Injecting input failed\n");
        return false;
    }

    fuzz_loop = true;
    return true;
}

void stop(bool crash) {
    if (!fuzz_loop)
        return;

    if (debug)
        printf("Stopping fuzz loop.\n");

    fuzz_loop = false;

    if (afl)
        afl_report(crash);
    else
        printf("Result: %s\n", crash ? "crash" : "no crash");
}
