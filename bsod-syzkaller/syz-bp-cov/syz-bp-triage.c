int main(int argc, char *argv[]) {
    if (argc < 2)
        return -1;
    asm("int $3" :: "a" (0x1337133713371300 | (*argv[1] == 0x31 ? 0x42 : 0x43)));
}
