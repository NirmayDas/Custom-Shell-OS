#include "parser.h"
#include <string.h>
#include <stdlib.h>

static int is_special(const char *tok){
    return tok && 
            (strcmp(tok, "<") == 0 ||
            strcmp(tok, ">") == 0 ||
            strcmp(tok, "2>") == 0 ||
            strcmp(tok, "|") == 0 ||
            strcmp(tok, "&") == 0);
}

static void free_temp_argv(char **arr, int count) {
    if (!arr) return;
    for (int i = 0; i < count; i++) free(arr[i]);
    free(arr);
}

int parse_command(const char *line, struct command *cmd) {
    if (line == NULL || cmd == NULL) {
        return 0;
    }

    cmd->argv=NULL; 
    cmd->pipe_argv=NULL;
    cmd->infile=cmd->outfile=cmd->errfile=NULL;
    cmd->pipe_infile=cmd->pipe_outfile=cmd->pipe_errfile=NULL;
    cmd->has_pipe=0; 
    cmd->background=0;

    //Make a copy of the line so that it is mutable (original is a readonly const)
    char *buf = strdup(line);
    if (buf == NULL) {
        return 0;
    }

    const char *delims = " \t\r\n";
    char *saveptr = NULL;
    char *curr_tok = strtok_r(buf, delims, &saveptr);

    // Dynamic arrays for left and right side
    char **left_argv = NULL;
    char **right_argv = NULL;
    int left_count = 0, right_count = 0;
    int left_capacity = 0, right_capacity = 0;
    int parsing_side = 0; // 0 = left, 1 = right

    int ampersand_seen = 0;  // flag to ensure & must be last

    while(curr_tok){
        if (ampersand_seen) {  // any token after & needs to result ina parse error
            free_temp_argv(left_argv, left_count);
            free_temp_argv(right_argv, right_count);
            free(buf); 
            free_command(cmd);
            return 0;
        }
        if(is_special(curr_tok)){
            if (strcmp(curr_tok, "<") == 0) {
                if(handle_input_redirection(cmd, curr_tok, &saveptr, delims, parsing_side) == 0){
                    free_temp_argv(left_argv, left_count);
                    free_temp_argv(right_argv, right_count);
                    free(buf);
                    free_command(cmd);
                    return 0;
                }
            } else if (strcmp(curr_tok, ">") == 0) {
                if(handle_output_redirection(cmd, curr_tok, &saveptr, delims, parsing_side) == 0){
                    free_temp_argv(left_argv, left_count);
                    free_temp_argv(right_argv, right_count);
                    free(buf);
                    free_command(cmd);
                    return 0;
                }
            } else if (strcmp(curr_tok, "2>") == 0) {
                if(handle_error_redirection(cmd, curr_tok, &saveptr, delims, parsing_side) == 0){
                    free_temp_argv(left_argv, left_count);
                    free_temp_argv(right_argv, right_count);
                    free(buf);
                    free_command(cmd);
                    return 0;
                }
            } else if (strcmp(curr_tok, "|") == 0) {
                if (cmd->background) {
                    free_temp_argv(left_argv, left_count);
                    free_temp_argv(right_argv, right_count);
                    free(buf); 
                    free_command(cmd);
                    return 0;
                }
                if(handle_pipe(cmd, curr_tok, &saveptr, delims, parsing_side) == 0){
                    free_temp_argv(left_argv, left_count);
                    free_temp_argv(right_argv, right_count);
                    free(buf);
                    free_command(cmd);
                    return 0;
                }
                parsing_side = 1; // Switch to right side for piping
            } else if (strcmp(curr_tok, "&") == 0) {
                if (cmd->has_pipe) {  // '|' and '&' cannot mix
                    free_temp_argv(left_argv, left_count);
                    free_temp_argv(right_argv, right_count);
                    free(buf); 
                    free_command(cmd);
                    return 0;
                }
                if(handle_background(cmd, curr_tok, &saveptr, delims, parsing_side) == 0){
                    free_temp_argv(left_argv, left_count);
                    free_temp_argv(right_argv, right_count);
                    free(buf);
                    free_command(cmd);
                    return 0;
                }
                ampersand_seen = 1; //set & flag
            }
        } else {
            if (parsing_side == 0) {
                // Left side
                if (left_count >= left_capacity) {
                    left_capacity = left_capacity ? left_capacity * 2 : 4;
                    left_argv = realloc(left_argv, sizeof(char*) * (left_capacity + 1));
                    if (!left_argv) {
                        free_temp_argv(left_argv, left_count);
                        free_temp_argv(right_argv, right_count);
                        free(buf);
                        free_command(cmd);
                        return 0;
                    }
                }
                left_argv[left_count] = strdup(curr_tok);
                if (!left_argv[left_count]) {
                    free_temp_argv(left_argv, left_count);
                    free_temp_argv(right_argv, right_count);
                    free(buf);
                    free_command(cmd);
                    return 0;
                }
                left_count++;
            } else {
                // Right side
                if (right_count >= right_capacity) {
                    right_capacity = right_capacity ? right_capacity * 2 : 4;
                    right_argv = realloc(right_argv, sizeof(char*) * (right_capacity + 1));
                    if (!right_argv) {
                        free_temp_argv(left_argv, left_count);
                        free_temp_argv(right_argv, right_count);
                        free(buf);
                        free_command(cmd);
                        return 0;
                    }
                }
                right_argv[right_count] = strdup(curr_tok);
                if (!right_argv[right_count]) {
                    free_temp_argv(left_argv, left_count);
                    free_temp_argv(right_argv, right_count);
                    free(buf);
                    free_command(cmd);
                    return 0;
                }
                right_count++;
            }
        }
        curr_tok = strtok_r(NULL, delims, &saveptr);
    }

    // Null  terminate arrays
    if (left_argv) {
        left_argv[left_count] = NULL;
        cmd->argv = left_argv;
    }
    if (right_argv) {
        right_argv[right_count] = NULL;
        cmd->pipe_argv = right_argv;
    }

    
    // post validation
    if (cmd->has_pipe && (!cmd->pipe_argv || !cmd->pipe_argv[0])) {
        free(buf); 
        free_command(cmd);
        return 0;
    }
    if (!cmd->argv || !cmd->argv[0]) {
        free(buf);
        free_command(cmd);
        return 0;
    }
    
    free(buf);
    return 1;
}

void free_command(struct command *cmd) {
    if (cmd == NULL) {
        return;
    }
    
    // Free left side argv
    if (cmd->argv != NULL) {
        for (int i = 0; cmd->argv[i] != NULL; i++) {
            free(cmd->argv[i]);
        }
        free(cmd->argv);
    }
    
    // Free left side redirection filenames
    if (cmd->infile) free(cmd->infile);
    if (cmd->outfile) free(cmd->outfile);
    if (cmd->errfile) free(cmd->errfile);
    
    // Free right side argv (if pipe exists)
    if (cmd->has_pipe && cmd->pipe_argv != NULL) {
        for (int i = 0; cmd->pipe_argv[i] != NULL; i++) {
            free(cmd->pipe_argv[i]);
        }
        free(cmd->pipe_argv);
    }
    
    // Free right side redirection filenames
    if (cmd->pipe_infile) free(cmd->pipe_infile);
    if (cmd->pipe_outfile) free(cmd->pipe_outfile);
    if (cmd->pipe_errfile) free(cmd->pipe_errfile);
}

// Helper function to handle input redirection (<)
int handle_input_redirection(struct command *cmd, char *curr_tok, char **saveptr, const char *delims, int parsing_side) {
    // Get the next token (filename)
    char *filename = strtok_r(NULL, delims, saveptr);
    if(filename == NULL){
        return 0; //missing filename
    }
    
    if (parsing_side == 0) {
        if(cmd->infile != NULL){
            return 0; //multiple input redirections
        }
        cmd->infile = strdup(filename);
        return cmd->infile != NULL;
    } else if (parsing_side == 1) {
        if(cmd->pipe_infile != NULL){
            return 0; //multiple input redirections
        }
        cmd->pipe_infile = strdup(filename);
        return cmd->pipe_infile != NULL;
    } else {
        return 0; //invalid parsing side
    }
}

// Helper function to handle output redirection (>)
int handle_output_redirection(struct command *cmd, char *curr_tok, char **saveptr, const char *delims, int parsing_side) {
    // Get the next token (filename)
    char *filename = strtok_r(NULL, delims, saveptr);
    if (filename == NULL){
        return 0; //missing filename
    }
    
    if (parsing_side == 0) {
        if(cmd->outfile != NULL){
            return 0; //multiple output redirections
        }
        cmd->outfile = strdup(filename);
        return cmd->outfile != NULL;
    } else if (parsing_side == 1) {
        if(cmd->pipe_outfile != NULL){
            return 0; //multiple output redirections
        }
        cmd->pipe_outfile = strdup(filename);
        return cmd->pipe_outfile != NULL;
    } else {
        return 0;
    }
}

// Helper function to handle error redirection (2>)
int handle_error_redirection(struct command *cmd, char *curr_tok, char **saveptr, const char *delims, int parsing_side) {
    // Get the next token (filename)
    char *filename = strtok_r(NULL, delims, saveptr);
    if (filename == NULL){
        return 0; //missing filename
    }
    
    if (parsing_side == 0) {
        if(cmd->errfile != NULL){
            return 0; //multiple error redirections
        }
        cmd->errfile = strdup(filename);
        return cmd->errfile != NULL;
    } else if (parsing_side == 1) {
        if(cmd->pipe_errfile != NULL){
            return 0; //multiple error redirections
        }
        cmd->pipe_errfile = strdup(filename);
        return cmd->pipe_errfile != NULL;
    } else {
        return 0;
    }
}

// Helper function to handle pipes (|)
int handle_pipe(struct command *cmd, char *curr_tok, char **saveptr, const char *delims, int parsing_side) {
    if(cmd->has_pipe) return 0; //if has_pipe is already been set and we are here, that means there are 2 pipes, we exit and fail
    cmd->has_pipe = 1;
    return 1;
}

// Helper function to handle background (&) - placeholder for now
int handle_background(struct command *cmd, char *curr_tok, char **saveptr, const char *delims, int parsing_side) {
    cmd->background = 1;
    return 1;
}

