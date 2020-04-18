#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include "config.h"
#include "metrics.h"


int signal_received = 0;

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        signal_received = signal;
    }
}

int main() {

    struct config vars;
    int env_errors = get_env_vars(&vars);
    if (env_errors != 0) {
        fprintf(stderr, "Could not load configuration (encountered %d fatal errors) - exiting.\n", env_errors);
        fflush(stderr);
        exit(1);
    }
    struct timespec sleep_duration;
    sleep_duration.tv_sec = vars.poll_time_millis / 1000;
    sleep_duration.tv_nsec = vars.poll_time_millis % 1000;


    struct sigaction action;
    action.sa_handler = &handle_signal;
    action.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &action, NULL) == -1) {
        fprintf(stderr, "Could not set handler for interrupt signal - errno = %d\n", errno);
        fflush(stderr);
        exit(1);
    }
    if (sigaction(SIGTERM, &action, NULL) == -1) {
        fprintf(stderr, "Could not set handler for interrupt signal - errno = %d\n", errno);
        fflush(stderr);
        exit(1);
    }

    while (signal_received == 0) {
        update_metrics(&vars);
        nanosleep(&sleep_duration, NULL);
    }

    printf("Received signal %d; exiting...\n", signal_received);
    fflush(stdout);

    return 0;
}


