#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include "parser.h"


int main(void){
    while(1){
        char *line = readline("# ");
        if(line == NULL){
            break;
        }

        char **argv;
        int argc = parse_argv(line, &argv);

        for (int i = 0; i < argc; i++) {
            printf("argv[%d] = %s\n", i, argv[i]);
        }

        for (int i = 0; i < argc; i++) {
            free(argv[i]);
        }
        free(argv);
        free(line);
    }
}