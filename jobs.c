#include "jobs.h"
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    (void)npids;

    // find a free slot
    int idx = -1;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].in_use) { 
            idx = i; 
            break; 
        }
    }
    if (idx < 0) return -1;

    jobs[idx].in_use = 1;
    jobs[idx].id = next_id++;
    jobs[idx].pgid = pgid;
    jobs[idx].pid = pids ? pids[0] : -1;
    jobs[idx].state = st;
    jobs[idx].cmdline = strdup(cmdline ? cmdline : "");
    if (!jobs[idx].cmdline) { 
        jobs[idx].in_use = 0; 
        return -1; 
    }

    // rotate markers: old + goes to - 
    for (int j = 0; j < MAX_JOBS; j++) {
        if (jobs[j].in_use){
            jobs[j].marker = 2;
        }
    }
    jobs[idx].marker = 1;
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
        if (!jobs[i].in_use){
            continue;
        }
        if (jobs[i].state == DONE) {
            printf("[%d]-  Done  %s\n", jobs[i].id, jobs[i].cmdline);
            jobs_remove(i);
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
        } else{
            mark = "-";
        }
        printf("[%d]%s  %s  %s\n", jobs[i].id, mark, st, jobs[i].cmdline);
    }
    putchar('\n');
}

int jobs_get_current(pid_t *pgid_out, job_state_t *state_out, const char **cmd_out, int *slot_out) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].in_use && jobs[i].marker == 1) {
            if (pgid_out){
                *pgid_out = jobs[i].pgid;
            }
            if (state_out){
                *state_out = jobs[i].state;
            }
            if (cmd_out){
                *cmd_out = jobs[i].cmdline;
            }   
            if (slot_out){
                *slot_out = i;
            }  
            return 0;
        }
    }
    return -1; // no current + job
}

void jobs_set_state(int slot, job_state_t st) {
    if (slot >= 0 && slot < MAX_JOBS && jobs[slot].in_use) {
        jobs[slot].state = st;
        // keep markers as same for now , current stays as +
    }
}

void jobs_remove(int slot) {
    if (slot < 0 || slot >= MAX_JOBS || !jobs[slot].in_use) return;

    int was_plus = (jobs[slot].marker == 1);

    free(jobs[slot].cmdline);
    jobs[slot].in_use = 0;

    if (was_plus) {
        int best = -1;
        int best_id = -1;
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].in_use && jobs[i].id > best_id) {
                best = i; 
                best_id = jobs[i].id;
            }
        }
        if (best != -1) {
            for (int i = 0; i < MAX_JOBS; i++){
                if (jobs[i].in_use){
                    jobs[i].marker = 2;
                }
            }
            jobs[best].marker = 1;
        }
    }
}


int jobs_get_current_stopped(pid_t *pgid_out, int *slot_out, const char **cmd_out, int *id_out) {
    // pref the current + if it's stopped
    // otherwise pick the highest id stooped job
    int best = -1;
    int best_id = -1;

    // first try the current +
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].in_use && jobs[i].marker == 1 && jobs[i].state == STOPPED) {
            best = i; 
            best_id = jobs[i].id;
            break;
        }
    }
    // if not found, pick most recent stopped by id
    if (best == -1) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].in_use && jobs[i].state == STOPPED && jobs[i].id > best_id) {
                best = i; 
                best_id = jobs[i].id;
            }
        }
    }

    if (best == -1){
        return -1;
    }

    if (pgid_out){
        *pgid_out = jobs[best].pgid;
    }
    if (id_out){
        *id_out = jobs[best].id;
    }
    if (cmd_out){
        *cmd_out = jobs[best].cmdline;
    }   
    if (slot_out){
        *slot_out  = best;
    }  
    
    return 0;
}

void jobs_mark_running(int slot) {
    if (slot < 0 || slot >= MAX_JOBS || !jobs[slot].in_use) return;
    for (int i = 0; i < MAX_JOBS; i++) if (jobs[i].in_use) jobs[i].marker = 2;
    jobs[slot].marker = 1;
    jobs[slot].state  = RUNNING;
}

