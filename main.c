#include "faststat.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static void timer_handler(int signum, siginfo_t *si, void *uc)
{
    struct faststat_env *env = si->si_value.sival_ptr;
    faststat_emit_line(env);
}

static void exec_timer(struct faststat_env *env, double interval)
{
    int signal_no = SIGRTMIN;

    struct sigaction act;
    act.sa_sigaction = timer_handler;
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    if (sigaction(signal_no, &act, NULL) < 0) {
        perror("sigaction()");
        exit(1);
    }

    timer_t tid;

    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = signal_no;
    sev.sigev_value.sival_ptr = env;
    if (timer_create(CLOCK_REALTIME, &sev, &tid)) {
        perror("timer_create");
        exit(1);
    }

    struct itimerspec itval;
    itval.it_value.tv_sec = 0;
    itval.it_value.tv_nsec = 1;  // non-zero value is necessary
    itval.it_interval.tv_sec = floor(interval);
    itval.it_interval.tv_nsec = (interval - floor(interval)) * 1000000000;
    if (timer_settime(tid, 0, &itval, NULL) < 0) {
        perror("timser_settime");
        exit(1);
    }
}

static _Noreturn void print_usage_to_exit(char *argv0)
{
    fprintf(stderr, "Usage: %s [-t nsecs] [fields...]\n", argv0);
    exit(1);
}

int main(int argc, char **argv)
{
    int opt;
    double interval = 0.5;
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't':
            interval = atof(optarg);
            break;
        default:
            print_usage_to_exit(argv[0]);
        }
    }

    // Parse command-line positional arguments as fields.
    FASTSTAT_FIELD *fields = NULL;
    int nfields = 0;
    if (argc - optind == 0) {
        // Passing no arguments means to print all fields.
        nfields = NUM_OF_FIELDS;
        fields = malloc(sizeof(FASTSTAT_FIELD) * nfields);
        for (int i = 0; i < NUM_OF_FIELDS; i++)
            fields[i] = i;
    }
    else {
        nfields = argc - optind;
        fields = malloc(sizeof(FASTSTAT_FIELD) * nfields);
        for (int i = 0; i < nfields; i++) {
            char *s = argv[optind + i];
            FASTSTAT_FIELD f = str2field(s);
            if (f == UNKNOWN) {
                fprintf(stderr, "Invalid field: %s\n", s);
                print_usage_to_exit(argv[0]);
            }
            fields[i] = f;
        }
    }
    assert(fields != NULL && nfields != 0);

    faststat_env *env = faststat_new_env(nfields, fields);
    free(fields);

    exec_timer(env, interval);
    while (1) {
        pause();
    }

    faststat_delete_env(env);
    return 0;
}
