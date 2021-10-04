#include <stdlib.h>
#include <string.h>

#define HYPERCALL_BUFFER    0x1337133713371338
#define HYPERCALL_TESTCASE  0x1337133713371337
#define LENGTH              0x10000

int main(int argv, char **argc) {
    // Allocate input buffer
    char *buffer = malloc(LENGTH);
    memset(buffer, 0, LENGTH);

    // Hypercall to signal buffer address and length
    asm("int $3" ::"a"(HYPERCALL_BUFFER), "b"(buffer), "c"(LENGTH));

    // Fuzzing loop
    while (1) {
        // Hypercall to request new test case
        asm("int $3" ::"a"(HYPERCALL_TESTCASE));

        // Call target function
        // target_function(buffer);
    }
}