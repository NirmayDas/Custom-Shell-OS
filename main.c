#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include "parser.h"
#include "exec.h"

int main(void) {
    signal(SIGINT, SIG_IGN); //ignore interrupt signal (crtl c)
    signal(SIGTSTP, SIG_IGN); //ignore stop signal (crtl z)

    while(1) {
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
            } else {
                run_simple_foreground(&cmd);
            }
            free_command(&cmd);
        } else {
            putchar('\n');
        }

        free(line);
    }
    return 0;
}
