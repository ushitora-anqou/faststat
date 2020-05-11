#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include <nvml.h>

struct cpu_stat {
    unsigned long user, nice, sys, idle, iowait, irq, softirq;
};
struct nvml_stat {
    unsigned int temp, power, usage;
    unsigned long long memory_used, memory_free, memory_total;
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

int main(int argc, char **argv)
{
    FILE *out = stdout;
    struct timespec nanosleep_ts = {0, 500000000};  // 0.5sec

    struct cpu_stat current, last;

    const unsigned int gpu_id = 0;
    struct nvml_stat nvml;
    nvmlInit();
    nvmlDevice_t nvml_dev;
    nvmlDeviceGetHandleByIndex(gpu_id, &nvml_dev);

    print_title(out);
    while (1) {
        last = current;
        read_cpu_stat(&current);
        read_nvml_stat(&nvml, nvml_dev);

        print_timestamp(out);
        print_cpu_stat(out, &current, &last);
        print_nvml_stat(out, &nvml);
        puts("");
        fflush(out);

        nanosleep(&nanosleep_ts, NULL);
    }

    return 0;
}
