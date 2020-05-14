#ifndef FASTSTAT_FASTSTAT_H
#define FASTSTAT_FASTSTAT_H

typedef struct faststat_env faststat_env;

faststat_env *faststat_new_env(void);
void faststat_delete_env(faststat_env *env);
void faststat_emit_line(faststat_env *env);

#endif
