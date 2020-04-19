#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include "config.h"
#include "metrics.h"


volatile int signalReceived = 0;

void handleSignal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        signalReceived = signal;
    }
}

int main() {

    struct config vars;
    int envErrors = getEnvVars(&vars);
    if (envErrors != 0) {
        fprintf(stderr, "Could not load configuration (encountered %d fatal errors) - exiting.\n", envErrors);
        fflush(stderr);
        exit(1);
    }
    struct timespec sleepDuration;
    sleepDuration.tv_sec = vars.pollTimeMillis / 1000;
    sleepDuration.tv_nsec = vars.pollTimeMillis % 1000;


    struct sigaction action;
    action.sa_handler = &handleSignal;
    action.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &action, NULL) == -1) {
        fprintf(stderr, "Could not set handler for SIGINT - errno = %d\n", errno);
        fflush(stderr);
        exit(1);
    }
    if (sigaction(SIGTERM, &action, NULL) == -1) {
        fprintf(stderr, "Could not set handler for SIGTERM - errno = %d\n", errno);
        fflush(stderr);
        exit(1);
    }

    while (signalReceived == 0) {
        updateMetrics(&vars);
        nanosleep(&sleepDuration, NULL);
    }

    printf("Received signal %d; exiting...\n", signalReceived);
    fflush(stdout);

    return 0;
}


