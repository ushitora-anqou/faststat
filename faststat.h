#ifndef FASTSTAT_FASTSTAT_H
#define FASTSTAT_FASTSTAT_H

typedef struct faststat_env faststat_env;

typedef enum {
    TIME,

    CPU_USER,
    CPU_NICE,
    CPU_SYS,
    CPU_IDLE,
    CPU_IOWAIT,
    CPU_IRQ,
    CPU_SOFTIRQ,
    CPU_STEAL,

    MEM_TOTAL,
    MEM_USED,
    MEM_FREE,
    MEM_SHARED,
    MEM_BUFF_CACHE,
    MEM_AVAILABLE,
    MEM_SWAP_TOTAL,
    MEM_SWAP_USED,
    MEM_SWAP_FREE,

    NVML_TEMP,
    NVML_POWER,
    NVML_USAGE,
    NVML_MEM_USED,
    NVML_MEM_FREE,
    NVML_MEM_TOTAL,

    UNKNOWN,  // MUST be the last field.
    NUM_OF_FIELDS = UNKNOWN,
} FASTSTAT_FIELD;

faststat_env *faststat_new_env(int nfields, FASTSTAT_FIELD fields[]);
void faststat_delete_env(faststat_env *env);
void faststat_emit_line(faststat_env *env);
FASTSTAT_FIELD str2field(const char *s);
const char *field2str(FASTSTAT_FIELD f);

#endif
