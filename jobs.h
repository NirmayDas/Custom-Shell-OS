#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

typedef enum { RUNNING, STOPPED, DONE } job_state_t;

void jobs_init(void);
int  jobs_add(pid_t pgid, const pid_t *pids, int npids, const char *cmdline, job_state_t st);
void jobs_reap_and_report(void);     // non-blocking wait loop and print Done
void jobs_print(void);               // main jobs command
int  jobs_fg(void);
int  jobs_bg(void);

#endif
