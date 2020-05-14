#include "faststat.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <time.h>

#include <nvml.h>

struct cpu_stat {
    unsigned long user, nice, sys, idle, iowait, irq, softirq, steal;
};
struct nvml_stat {
    unsigned int temp, power, usage;
    unsigned long long memory_used, memory_free, memory_total;
};
struct faststat_env {
    FILE *out;
    unsigned long long nlines;

    size_t nfields;
    FASTSTAT_FIELD *fields;

    struct {
        double user, nice, sys, idle, iowait, irq, softirq, steal;
        struct cpu_stat last;
    } cpu;

    struct {
        nvmlDevice_t dev;
        struct nvml_stat current;
    } nvml;
};

faststat_env *faststat_new_env(int nfields, FASTSTAT_FIELD fields[])
{
    faststat_env *env = malloc(sizeof(faststat_env));

    env->out = stdout;
    env->nlines = 0;

    size_t size = sizeof(FASTSTAT_FIELD) * nfields;
    env->fields = malloc(size);
    memcpy(env->fields, fields, size);
    env->nfields = nfields;

    const unsigned int gpu_id = 0;
    nvmlInit();
    nvmlDeviceGetHandleByIndex(gpu_id, &env->nvml.dev);

    memset(&env->cpu.last, 0, sizeof(env->cpu.last));

    return env;
}

void faststat_delete_env(faststat_env *env)
{
    free(env->fields);
    free(env);
}

static _Noreturn void failwith(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(1);
}

static void print_punctuation(faststat_env *env)
{
    fprintf(env->out, ",");
}

static void print_title(faststat_env *env)
{
    for (size_t i = 0; i < env->nfields; i++) {
        if (i != 0)
            print_punctuation(env);

        FASTSTAT_FIELD field = env->fields[i];
        fputs(field2str(field), env->out);
    }
    fputs("\n", env->out);
}

static void print_timestamp(faststat_env *env)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct tm *t = localtime(&now.tv_sec);
    fprintf(env->out, "%04d-%02d-%02d %02d:%02d:%02d.%03ld", t->tm_year + 1900,
            t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,
            now.tv_usec / 1000);
}

static void read_field_cpu(faststat_env *env)
{
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp)
        failwith("Can't open /proc/stat");
    struct cpu_stat current;
    int n = fscanf(fp, "cpu  %lu %lu %lu %lu %lu %lu %lu %lu", &current.user,
                   &current.nice, &current.sys, &current.idle, &current.iowait,
                   &current.irq, &current.softirq, &current.steal);
    if (n == EOF || n != 8)
        failwith("Unknown format of /proc/stat");
    fclose(fp);

    struct cpu_stat *last = &env->cpu.last;
    double user = current.user - last->user, nice = current.nice - last->nice,
           sys = current.sys - last->sys, idle = current.idle - last->idle,
           iowait = current.iowait - last->iowait,
           irq = current.irq - last->irq,
           softirq = current.softirq - last->softirq,
           steal = current.steal - last->steal;
    double total = user + nice + sys + idle + iowait + irq + softirq + steal;

    if (total < DBL_EPSILON) {
        env->cpu.user = 0;
        env->cpu.nice = 0;
        env->cpu.sys = 0;
        env->cpu.idle = 0;
        env->cpu.iowait = 0;
        env->cpu.irq = 0;
        env->cpu.softirq = 0;
        env->cpu.steal = 0;
    }
    else {
        env->cpu.user = user / total * 100;
        env->cpu.nice = nice / total * 100;
        env->cpu.sys = sys / total * 100;
        env->cpu.idle = idle / total * 100;
        env->cpu.iowait = iowait / total * 100;
        env->cpu.irq = irq / total * 100;
        env->cpu.softirq = softirq / total * 100;
        env->cpu.steal = steal / total * 100;
    }

    *last = current;
}

#define DEF_PRINT_FIELD_CPU(name)                         \
    static void print_field_cpu_##name(faststat_env *env) \
    {                                                     \
        fprintf(env->out, "%2.02f", env->cpu.name);       \
    }
DEF_PRINT_FIELD_CPU(user)
DEF_PRINT_FIELD_CPU(nice)
DEF_PRINT_FIELD_CPU(sys)
DEF_PRINT_FIELD_CPU(idle)
DEF_PRINT_FIELD_CPU(iowait)
DEF_PRINT_FIELD_CPU(irq)
DEF_PRINT_FIELD_CPU(softirq)
DEF_PRINT_FIELD_CPU(steal)
#undef DEF_PRINT_FIELD_CPU

static void read_field_nvml(faststat_env *env)
{
    nvmlDeviceGetTemperature(env->nvml.dev, NVML_TEMPERATURE_GPU,
                             &env->nvml.current.temp);
    nvmlDeviceGetPowerUsage(env->nvml.dev, &env->nvml.current.power);

    nvmlUtilization_t utilization;
    nvmlDeviceGetUtilizationRates(env->nvml.dev, &utilization);
    env->nvml.current.usage = utilization.gpu;

    nvmlMemory_t memory;
    nvmlDeviceGetMemoryInfo(env->nvml.dev, &memory);
    env->nvml.current.memory_free = memory.free;
    env->nvml.current.memory_used = memory.used;
    env->nvml.current.memory_total = memory.total;
}

static void print_field_nvml_temp(faststat_env *env)
{
    fprintf(env->out, "%u", env->nvml.current.temp);
}

static void print_field_nvml_power(faststat_env *env)
{
    fprintf(env->out, "%.02f", (float)env->nvml.current.power / 1000);
}

static void print_field_nvml_usage(faststat_env *env)
{
    fprintf(env->out, "%u", env->nvml.current.usage);
}

static void print_field_nvml_mem_used(faststat_env *env)
{
    fprintf(env->out, "%.02f",
            (float)(env->nvml.current.memory_used) / 1024 / 1024);
}

static void print_field_nvml_mem_free(faststat_env *env)
{
    fprintf(env->out, "%.02f",
            (float)(env->nvml.current.memory_free) / 1024 / 1024);
}

static void print_field_nvml_mem_total(faststat_env *env)
{
    fprintf(env->out, "%.02f",
            (float)(env->nvml.current.memory_total) / 1024 / 1024);
}

static void print_line(faststat_env *env)
{
    if (env->nlines == 0)
        print_title(env);

    for (size_t i = 0; i < env->nfields; i++) {
        if (i != 0)
            print_punctuation(env);

        FASTSTAT_FIELD field = env->fields[i];
        switch (field) {
        case TIME:
            print_timestamp(env);
            break;

#define CASE_CPU(capital_name, name) \
    case CPU_##capital_name:         \
        print_field_cpu_##name(env); \
        break;
            CASE_CPU(USER, user)
            CASE_CPU(NICE, nice)
            CASE_CPU(SYS, sys)
            CASE_CPU(IDLE, idle)
            CASE_CPU(IOWAIT, iowait)
            CASE_CPU(IRQ, irq)
            CASE_CPU(SOFTIRQ, softirq)
            CASE_CPU(STEAL, steal)
#undef CASE_CPU

#define CASE_NVML(capital_name, name) \
    case NVML_##capital_name:         \
        print_field_nvml_##name(env); \
        break;
            CASE_NVML(TEMP, temp)
            CASE_NVML(POWER, power)
            CASE_NVML(USAGE, usage)
            CASE_NVML(MEM_USED, mem_used)
            CASE_NVML(MEM_FREE, mem_free)
            CASE_NVML(MEM_TOTAL, mem_total)
#undef CASE_NVML

        default:
            assert(0);
        }
    }

    fputs("\n", env->out);
    fflush(env->out);
}

void faststat_emit_line(faststat_env *env)
{
    read_field_cpu(env);
    read_field_nvml(env);
    print_line(env);
    env->nlines++;
}

const char *field2str(FASTSTAT_FIELD f)
{
    switch (f) {
    case TIME:
        return "time";
    case CPU_USER:
        return "cpu.user";
    case CPU_NICE:
        return "cpu.nice";
    case CPU_SYS:
        return "cpu.sys";
    case CPU_IDLE:
        return "cpu.idle";
    case CPU_IOWAIT:
        return "cpu.iowait";
    case CPU_IRQ:
        return "cpu.irq";
    case CPU_SOFTIRQ:
        return "cpu.softirq";
    case CPU_STEAL:
        return "cpu.steal";
    case NVML_TEMP:
        return "nvml.temp";
    case NVML_POWER:
        return "nvml.power";
    case NVML_USAGE:
        return "nvml.usage";
    case NVML_MEM_USED:
        return "nvml.mem.used";
    case NVML_MEM_FREE:
        return "nvml.mem.free";
    case NVML_MEM_TOTAL:
        return "nvml.mem.total";
    default:
        assert(0);
    }
}

FASTSTAT_FIELD str2field(const char *s)
{
    if (strcmp(s, "time") == 0)
        return TIME;
    if (strcmp(s, "cpu.user") == 0)
        return CPU_USER;
    if (strcmp(s, "cpu.nice") == 0)
        return CPU_NICE;
    if (strcmp(s, "cpu.sys") == 0)
        return CPU_SYS;
    if (strcmp(s, "cpu.idle") == 0)
        return CPU_IDLE;
    if (strcmp(s, "cpu.iowait") == 0)
        return CPU_IOWAIT;
    if (strcmp(s, "cpu.irq") == 0)
        return CPU_IRQ;
    if (strcmp(s, "cpu.softirq") == 0)
        return CPU_SOFTIRQ;
    if (strcmp(s, "cpu.steal") == 0)
        return CPU_STEAL;
    if (strcmp(s, "nvml.temp") == 0)
        return NVML_TEMP;
    if (strcmp(s, "nvml.power") == 0)
        return NVML_POWER;
    if (strcmp(s, "nvml.usage") == 0)
        return NVML_USAGE;
    if (strcmp(s, "nvml.mem.used") == 0)
        return NVML_MEM_USED;
    if (strcmp(s, "nvml.mem.free") == 0)
        return NVML_MEM_FREE;
    if (strcmp(s, "nvml.mem.total") == 0)
        return NVML_MEM_TOTAL;

    return UNKNOWN;
}
