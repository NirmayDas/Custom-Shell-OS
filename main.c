#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include "parser.h"
#include "exec.h"
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include "jobs.h"

int main(void) {
    signal(SIGINT, SIG_IGN); //ignore interrupt signal (crtl c)
    signal(SIGTSTP, SIG_IGN); //ignore stop signal (crtl z)
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    jobs_init();

    // this makes sure the shell is its own process group leader
    setpgid(0, 0);
    // this makes sure the shell's pgid owns the terminal
    tcsetpgrp(STDIN_FILENO, getpgrp());
    exec_set_shell_pgid(getpgrp());


    while(1) {
        jobs_reap_and_report();
        char *line = readline("# ");
        if (line == NULL){
            break; // Ctrl-D will exit shell
        }
        if (line[0] == '\0') { 
            free(line); 
            continue; 
        }

        struct command cmd;
        if (parse_command(line, &cmd)) {
            if (cmd.has_pipe) {
                run_single_pipeline(&cmd);
            } else if (cmd.argv && cmd.argv[0] && strcmp(cmd.argv[0], "jobs") == 0) {
                jobs_print();
            } else if (cmd.argv && cmd.argv[0] && strcmp(cmd.argv[0], "fg") == 0){
                pid_t pgid; 
                job_state_t st; 
                const char *cmdtxt; 
                int slot;
                if (jobs_get_current(&pgid, &st, &cmdtxt, &slot) == 0) {
                    exec_foreground_job(pgid, slot, st, cmdtxt);
                } else {
                    // no current job
                    putchar('\n');
                }
            } else if (cmd.argv && cmd.argv[0] && strcmp(cmd.argv[0], "bg") == 0){
                pid_t pgid; 
                int slot; 
                const char *txt; 
                int id;
                if (jobs_get_current_stopped(&pgid, &slot, &txt, &id) == 0) {
                    // resume in background and send SIGCONT to the whole process group
                    kill(-pgid, SIGCONT);
                    jobs_mark_running(slot);
                    printf("[%d]+  Running  %s \n", id, txt);
                } else {
                    //nothing to bg
                    putchar('\n');
                }
            } else if (cmd.background){
                run_single_background(&cmd, line);
            } else {
                run_simple_foreground(&cmd, line);
            }
            free_command(&cmd);
        } else {
            putchar('\n');
        }

        free(line);
    }
    return 0;
}
