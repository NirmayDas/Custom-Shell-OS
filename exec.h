// exec.h — Step 2 scaffolding
#ifndef EXEC_H
#define EXEC_H

#include "parser.h"

// Run a single foreground command (no redirs/pipes/bg yet).
// Returns 1 if it handled the command path, else 0.
int run_simple_foreground(struct command *cmd);

// handles piping: cmd1 | cmd2 
int run_single_pipeline(struct command *cmd);


#endif /* EXEC_H */
