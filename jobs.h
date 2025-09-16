#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

typedef enum { RUNNING, STOPPED, DONE } job_state_t;

void jobs_init(void);
int  jobs_add(pid_t pgid, const pid_t *pids, int npids, const char *cmdline, job_state_t st);
void jobs_reap_and_report(void);
void jobs_print(void);
int jobs_get_current(pid_t *pgid_out, job_state_t *state_out, const char **cmd_out, int *slot_out);
void jobs_set_state(int slot, job_state_t st);
void jobs_remove(int slot);

// Get the most-recent stopped job return 0/-1
int jobs_get_current_stopped(pid_t *pgid_out, int *slot_out, const char **cmd_out, int *id_out);

// set RUNNING and make it current +
void jobs_mark_running(int slot);

int jobs_has_capacity(void);


#endif
