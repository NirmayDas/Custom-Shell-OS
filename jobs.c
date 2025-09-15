#include "jobs.h"
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX_JOBS 20

typedef struct {
    int in_use;
    int id;
    pid_t pgid;
    pid_t pid;
    job_state_t state;  // RUNNING/STOPPED/DONE
    char *cmdline;      // original command ran
    int marker;
} job_t;

static job_t jobs[MAX_JOBS];
static int next_id = 1;

void jobs_init(void) {
    memset(jobs, 0, sizeof(jobs));
    next_id = 1;
}

static int find_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].in_use && jobs[i].pid == pid) return i;
    }
    return -1;
}


int jobs_add(pid_t pgid, const pid_t *pids, int npids, const char *cmdline, job_state_t st) {
    (void)npids; // bg is single child; fg-stopped we still record 1 pid

    // find a free slot
    int idx = -1;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].in_use) { 
            idx = i; 
            break; 
        }
    }
    if (idx < 0) return -1;

    jobs[idx].in_use  = 1;
    jobs[idx].id      = next_id++;
    jobs[idx].pgid    = pgid;
    jobs[idx].pid     = pids ? pids[0] : -1;
    jobs[idx].state   = st;
    jobs[idx].cmdline = strdup(cmdline ? cmdline : "");
    if (!jobs[idx].cmdline) { 
        jobs[idx].in_use = 0; 
        return -1; 
    }

    // rotate markers: old '+' -> '-', others -> none
    for (int j = 0; j < MAX_JOBS; j++) {
        if (!jobs[j].in_use) continue;
        jobs[j].marker = (jobs[j].marker == 1) ? 2 : 0;
    }
    jobs[idx].marker = 1; // this new job is the current '+'

    return jobs[idx].id;
}

void jobs_reap_and_report(void) {
    int st;
    pid_t pid;
    while ((pid = waitpid(-1, &st, WNOHANG)) > 0) {
        int idx = find_by_pid(pid);
        if (idx < 0) continue; // not a tracked bg job (e.g., fg child)
        // Mark done if exited or killed by signal
        if (WIFEXITED(st) || WIFSIGNALED(st)) {
            jobs[idx].state = DONE;
        }
    }

    // Print and remove DONE jobs
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].in_use) continue;
        if (jobs[i].state == DONE) {
            printf("Done\t[%d] %s\n", jobs[i].id, jobs[i].cmdline);
            free(jobs[i].cmdline);
            jobs[i].in_use = 0;
        }
    }
}

void jobs_print(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if(!jobs[i].in_use){
            continue;
        }

        const char *st;
        if (jobs[i].state == RUNNING) {
            st = "Running";
        } else if (jobs[i].state == STOPPED) {
            st = "Stopped";
        } else {
            st = "Done";
        }
        const char *mark;
        if(jobs[i].marker == 1){
            mark = "+";
        } else if (jobs[i].marker == 2){
            mark = "-";
        } else {
            mark = "";
        }

        printf("[%d]%s  %s  %s\n", jobs[i].id, mark, st, jobs[i].cmdline);
    }
    putchar('\n');
}

int  jobs_fg(void) { 
    return 0;
}

int  jobs_bg(void) { 
    return 0; 
}