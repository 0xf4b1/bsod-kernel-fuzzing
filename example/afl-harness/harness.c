#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

#define HYPERCALL_BUFFER    0x1337133713371338
#define HYPERCALL_TESTCASE  0x1337133713371337

void main(int argc, char *argv[]) {
    int fd = open("/dev/testdriver", O_RDWR);
    if (fd == -1) {
        printf("Can not open device\n");
        exit(-1);
    }

    char *buffer = malloc(sizeof(int));
    memset(buffer, 0, sizeof(int));

    asm("int $3" ::"a"(HYPERCALL_BUFFER), "b"(buffer), "c"(sizeof(int)));
    for (;;) {
        asm("int $3" ::"a"(HYPERCALL_TESTCASE));
        int ret = ioctl(fd, _IOC(_IOC_READ|_IOC_WRITE, 0x46, 0x0, sizeof(int)), buffer);
    }
}
