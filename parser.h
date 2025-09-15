#ifndef PARSER_H
#define PARSER_H

// structure to represent a complete command with redirections and pipes
struct command {
    char **argv;                                        // left side command + args (NULL-terminated)
    char *infile, *outfile, *errfile;                  // left side redirections
    int has_pipe;
    char **pipe_argv;                                  // right side command + args (NULL-terminated)
    char *pipe_infile, *pipe_outfile, *pipe_errfile;   // right side redirections
    int background;
};

// parses a complete command line into a command structure
// returns 1 on success, 0 on failure
// caller must call free_command() to clean up memory
int parse_command(const char *line, struct command *cmd);

// Frees memory allocated for a command structure
void free_command(struct command *cmd);

// Helper functions for parsing special tokens
int handle_input_redirection(struct command *cmd, char *curr_tok, char **saveptr, const char *delims, int parsing_side);
int handle_output_redirection(struct command *cmd, char *curr_tok, char **saveptr, const char *delims, int parsing_side);
int handle_error_redirection(struct command *cmd, char *curr_tok, char **saveptr, const char *delims, int parsing_side);
int handle_pipe(struct command *cmd, char *curr_tok, char **saveptr, const char *delims, int parsing_side);
int handle_background(struct command *cmd, char *curr_tok, char **saveptr, const char *delims, int parsing_side);

#endif // PARSER_H