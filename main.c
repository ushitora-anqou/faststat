#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <nvml.h>

struct cpu_stat {
    unsigned long user, nice, sys, idle, iowait, irq, softirq;
};
struct nvml_stat {
    unsigned int temp, power, usage;
    unsigned long long memory_used, memory_free, memory_total;
};
struct faststat_env {
    FILE *out;
    nvmlDevice_t nvml_dev;
    struct cpu_stat cpu_last;
};

_Noreturn void failwith(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(1);
}

void read_cpu_stat(struct cpu_stat *cpu)
{
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp)
        failwith("Can't open /proc/stat");
    int n =
        fscanf(fp, "cpu  %lu %lu %lu %lu %lu %lu %lu", &cpu->user, &cpu->nice,
               &cpu->sys, &cpu->idle, &cpu->iowait, &cpu->irq, &cpu->softirq);
    if (n == EOF || n != 7)
        failwith("Unknown format of /proc/stat");
    fclose(fp);
}

void print_title(FILE *out)
{
    fprintf(out,
            "time,cpu.user,cpu.nice,cpu.sys,cpu.idle,cpu.iowait,cpu.irq,cpu."
            "softirq,nvml.temp,nvml.power,nvml.usage,nvml.mem.used,nvml.mem."
            "free,nvml.mem.total\n");
}

void print_timestamp(FILE *out)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct tm *t = localtime(&now.tv_sec);
    fprintf(out, "%04d-%02d-%02d %02d:%02d:%02d.%03ld", t->tm_year + 1900,
            t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,
            now.tv_usec / 1000);
}

void print_cpu_stat(FILE *out, const struct cpu_stat *const current,
                    const struct cpu_stat *const last)
{
    unsigned long user = current->user - last->user,
                  nice = current->nice - last->nice,
                  sys = current->sys - last->sys,
                  idle = current->idle - last->idle,
                  iowait = current->iowait - last->iowait,
                  irq = current->irq - last->irq,
                  softirq = current->softirq - last->softirq;
    unsigned long total = user + nice + sys + idle + iowait + irq + softirq;

    if (total == 0)
        return;

#define FPRINTF(name) fprintf(out, ",%.02f", (float)name / total * 100);
    FPRINTF(user);
    FPRINTF(nice);
    FPRINTF(sys);
    FPRINTF(idle);
    FPRINTF(iowait);
    FPRINTF(irq);
    FPRINTF(softirq);
#undef FPRINTF
}

void read_nvml_stat(struct nvml_stat *nvml, nvmlDevice_t device)
{
    nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &nvml->temp);
    nvmlDeviceGetPowerUsage(device, &nvml->power);

    nvmlUtilization_t utilization;
    nvmlDeviceGetUtilizationRates(device, &utilization);
    nvml->usage = utilization.gpu;

    nvmlMemory_t memory;
    nvmlDeviceGetMemoryInfo(device, &memory);
    nvml->memory_free = memory.free;
    nvml->memory_used = memory.used;
    nvml->memory_total = memory.total;
}

void print_nvml_stat(FILE *out, struct nvml_stat *nvml)
{
    fprintf(out, ",%u", nvml->temp);
    fprintf(out, ",%.02f", (float)nvml->power / 1000);
    fprintf(out, ",%u", nvml->usage);
    fprintf(out, ",%.02f", (float)(nvml->memory_used) / 1024 / 1024);
    fprintf(out, ",%.02f", (float)(nvml->memory_free) / 1024 / 1024);
    fprintf(out, ",%.02f", (float)(nvml->memory_total) / 1024 / 1024);
}

void faststat_init(struct faststat_env *env)
{
    env->out = stdout;

    const unsigned int gpu_id = 0;
    nvmlInit();
    nvmlDeviceGetHandleByIndex(gpu_id, &env->nvml_dev);

    memset(&env->cpu_last, 0, sizeof(env->cpu_last));
}

void faststat_timer_handler(int signum, siginfo_t *si, void *uc)
{
    struct faststat_env *env = si->si_value.sival_ptr;
    struct cpu_stat cpu;
    struct nvml_stat nvml;

    read_cpu_stat(&cpu);
    read_nvml_stat(&nvml, env->nvml_dev);

    print_timestamp(env->out);
    print_cpu_stat(env->out, &cpu, &env->cpu_last);
    print_nvml_stat(env->out, &nvml);
    puts("");
    fflush(env->out);

    env->cpu_last = cpu;
}

void faststat_exec_timer(struct faststat_env *env)
{
    int signal_no = SIGRTMIN;

    struct sigaction act;
    act.sa_sigaction = faststat_timer_handler;
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
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = 500000000;  // 0.5sec
    if (timer_settime(tid, 0, &itval, NULL) < 0) {
        perror("timser_settime");
        exit(1);
    }
}

int main(int argc, char **argv)
{
    struct faststat_env env;
    faststat_init(&env);
    print_title(env.out);
    faststat_exec_timer(&env);

    while (1) {
        pause();
    }

    return 0;
}
