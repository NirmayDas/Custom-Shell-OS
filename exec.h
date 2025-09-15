#ifndef EXEC_H
#define EXEC_H

#include "parser.h"
#include <sys/types.h>

// Run a single foreground command (no redirs/pipes/bg yet).
// Returns 1 if it handled the command path, else 0.
int run_simple_foreground(struct command *cmd, const char *cmdline);

// handles piping: cmd1 | cmd2 
int run_single_pipeline(struct command *cmd);

//used to restore control
void exec_set_shell_pgid(pid_t pgid);

//run a single background command (no pipes)
int run_single_background(struct command *cmd, const char *cmdline);


#endif /* EXEC_H */
