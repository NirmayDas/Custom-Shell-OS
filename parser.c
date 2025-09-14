#include "parser.h"
#include <string.h>
#include <stdlib.h>

//Helper function to identify special tokens
static int is_special(const char *tok){
    return tok && 
            (strcmp(tok, "<") == 0 ||
            strcmp(tok, ">") == 0 ||
            strcmp(tok, "2>") == 0 ||
            strcmp(tok, "|") == 0 ||
            strcmp(tok, "&") == 0);
}

int parse_argv(const char *line, char ***argv_original) {
    if (argv_original) {
        /*
            We make argv_original's pointer point to NULL so if the function randomly
            exits early, the caller's pointer won't be left unititialzied and pointing to garbage
            Ex: if the caller tries to use or free argv without this check (from main) that will result in 
            undefined behavior
        */

        *argv_original = NULL;
    }

    if (line == NULL){
        return 0;
    }

    //Make a copy of the line so that it is mutable (original is a readonly const)
    char *buf = strdup(line);
    if (buf == NULL) {
        return 0;
    }

    //First we need to count the number of tokens so we can parse through each & allocate space
    int count = 0;
    const char *delims = " \t\r\n";
    char *saveptr = NULL;
    char *curr_tok = strtok_r(buf, delims, &saveptr);

    while(curr_tok){
        if(!is_special(curr_tok)){
            count++;
            curr_tok = strtok_r(NULL, delims, &saveptr);
        } else {
            break;
        }
    }

    char **argv_local = malloc(sizeof(char*) * (count + 1));
    if(argv_local == NULL){
        free(buf);
        return 0;
    }

    strcpy(buf, line); //This method doesn't allocate new memory, it just overrwrites vs. strdup allocates
    saveptr = NULL;
    curr_tok = strtok_r(buf, delims, &saveptr);

    int i = 0;
    while(curr_tok && (i < count)){
        if(is_special(curr_tok)){
            break;
        }
        argv_local[i] = strdup(curr_tok);
        i++;
        curr_tok = strtok_r(NULL, delims, &saveptr);
    }
    argv_local[i] = NULL;

    *argv_original = argv_local;
    free(buf);
    return count;
}
