#include "private.h"

static void usage(void) {
    printf("Inputs required for FUZZING step:\n");
    printf("\t  --socket <path to KVMi socket>\n");
    printf("\t  --input <path to input file> or @@ with AFL\n");
    printf("\t  --input-limit <limit input size>\n");
    printf("\t  --address <kernel virtual address to inject input to>\n");
    printf("\t  --json <path to kernel debug json>\n");
    printf("\t  --module <kernel virtual address of kernel module>\n");
    printf("\t  --start <module offset to start fuzzing>\n");
    printf("\t  --target <module offset to end fuzzing>\n");
    printf("\tOptional inputs:\n");
    printf("\t  --limit <limit FUZZING execution to # of CF instructions>\n");
    printf("\t  --breakpoints <file that contains addresses of CF instructions>\n");
    printf("\t  --coverage <full|block|edge coverage in breakpoint mode>\n");
    printf("\t  --pid\n");
    printf("\t  --reconnect\n");
    printf("\t  --debug\n");
    printf("\t  --logfile <path to logfile>\n");
}

int main(int argc, char **argv) {
    char *logfile = NULL;
    int c, out = 0, long_index = 0;
    const struct option long_opts[] = {{"help", no_argument, NULL, 'h'},
                                       {"socket", required_argument, NULL, 'S'},
                                       {"json", required_argument, NULL, 'j'},
                                       {"input", required_argument, NULL, 'f'},
                                       {"address", required_argument, NULL, 'a'},
                                       {"module", required_argument, NULL, 'm'},
                                       {"start", required_argument, NULL, 's'},
                                       {"target", required_argument, NULL, 't'},
                                       {"limit", required_argument, NULL, 'l'},
                                       {"debug", no_argument, NULL, 'v'},
                                       {"logfile", required_argument, NULL, 'F'},
                                       {"breakpoints", required_argument, NULL, 'b'},
                                       {"coverage", required_argument, NULL, 'c'},
                                       {"pid", no_argument, NULL, 'p'},
                                       {"reconnect", no_argument, NULL, 'r'},
                                       {NULL, 0, NULL, 0}};
    const char *opts = "S:j:f:a:m:s:t:l:F:b:c:p:vhO";
    limit = ~0;
    cc = 0xcc;
    input_path = NULL;
    input_size = 0;
    input_limit = 0;
    mode = DYNAMIC;
    reconnect = false;

    module_start = 0;
    start_offset = 0;
    target_offset = 0;

    while ((c = getopt_long(argc, argv, opts, long_opts, &long_index)) != -1) {
        switch (c) {
        case 'S':
            socket = optarg;
            break;
        case 'j':
            json = optarg;
            break;
        case 'f':
            input_path = optarg;
            break;
        case 'a':
            address = strtoull(optarg, NULL, 0);
            break;
        case 'm':
            module_start = strtoull(optarg, NULL, 0);
            break;
        case 's':
            start_offset = strtoull(optarg, NULL, 0);
            break;
        case 't':
            target_offset = strtoull(optarg, NULL, 0);
            break;
        case 'l':
            limit = strtoull(optarg, NULL, 0);
            break;
        case 'v':
            debug = true;
            break;
        case 'F':
            logfile = optarg;
            break;
        case 'b':
            bp_file = optarg;
            break;
        case 'c':
            if (!strcmp(optarg, "block"))
                mode = BLOCK;
            else if (!strcmp(optarg, "edge"))
                mode = EDGE;
            else if (!strcmp(optarg, "full"))
                mode = FULL;
            break;
        case 'p':
            trace_pid = true;
            break;
        case 'r':
            reconnect = true;
            break;
        case 'h': /* fall-through */
        default:
            usage();
            return -1;
        };
    }

    if (!json || !input_path) {
        usage();
        return -1;
    }

    if (logfile) {
        out = open(logfile, O_RDWR | O_CREAT | O_APPEND, 0600);
        if (-1 == dup2(out, fileno(stdout)))
            return -1;
        if (-1 == dup2(out, fileno(stderr)))
            return -1;
    }

    char filename[20];
    snprintf(filename, 20, "coverage%c.txt", socket[strlen(socket) - 1]);
    coverage_fp = fopen(filename, "w");
    if (!coverage_fp) {
        fprintf(stderr, "Can not open coverage output\n");
        return -1;
    }

    setup_handlers();
    afl_setup();

    for (;;) {
        if (debug)
            printf("############ START ################\n");

        if (!setup_vmi(&vmi, socket, json)) {
            fprintf(stderr, "Unable to start VMI on domain\n");
            sleep(10);
            continue;
        }

        if (VMI_FAILURE == vmi_pause_vm(vmi)) {
            printf("Failed to pause vm\n");
            goto done;
        }

        if (!init_tracer(vmi)) {
            fprintf(stderr, "Setup trace failed\n");
            goto done;
        }

        if (afl)
            printf("Running in AFL mode\n");
        else
            printf("Running in standalone mode\n");

        loop(vmi);

        if (!reconnect || interrupted)
            break;

        vmi_destroy(vmi);

        failure = false;
        waiting = true;
        coverage_enabled = false;
        while (waiting) {
            printf("Waiting for signal ...\n");
            sleep(1);
        }
        printf("Got module address: %lx\n", module_start);
    }

    teardown();

done:
    vmi_destroy(vmi);
    cs_close(&cs_handle);
    if (input_file)
        fclose(input_file);

    if (debug)
        printf(" ############ DONE ##############\n");
    if (logfile)
        close(out);

    return 0;
}
