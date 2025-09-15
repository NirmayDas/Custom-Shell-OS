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
