#include <signal.h>
#include <stdbool.h>
#include <stddef.h>

static struct sigaction act;
extern int interrupted;
extern unsigned long module_start;
extern bool waiting;

static void close_handler(int sig) { interrupted = sig; }

void sighand(int signo, siginfo_t *info, void *extra) {
    module_start = info->si_value.sival_ptr;
    waiting = false;
}

void setup_handlers(void) {
    act.sa_handler = close_handler;
    sigemptyset(&act.sa_mask);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGALRM, &act, NULL);

    struct sigaction action;
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = &sighand;
    sigaction(SIGUSR2, &action, NULL);
}
