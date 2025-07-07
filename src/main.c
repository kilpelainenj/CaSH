#include "main.h"
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>


//extern int do_exit(int, char**);
//extern int do_cd(int, char**);
extern int do_ls(int, char**);
extern int do_cat(int, char**);
extern int do_cd(int, char**);

builtin_t builtins[] = {
    //{ "exit", do_exit},
    //{ "cd", do_cd},
    { "ls", do_ls},
    { "cat", do_cat},
    { "cd", do_cd},
    { NULL, NULL}
};



int main(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while (1) {
        char buf[PATH_MAX];
        if (getcwd(buf, sizeof(buf)) == NULL) {
            perror("getcwd");
        }
        printf("cash> %s ", buf);
        fflush(stdout);
        nread = getline(&line, &len, stdin);
        if (nread == -1) {
            break;
        } else if (nread == 0) {
            continue;
        }

        if (line[nread-1] == '\n') {
            line[nread-1] = '\0';
        }

        // Parse the command
        char *argv[64];
        int argc = 0;
        char *tok = strtok(line, " \t");
        while(tok) {
            argv[argc++] = tok;
            tok = strtok(NULL, " \t");
        }
        argv[argc] = NULL;

        // Iterate over our builtins table and execute the called command
        // Stops when b->name is NULL and it evaluates to false
        for (builtin_t *b = builtins; b->name; b++) {
            if (strcmp(argv[0], b->name) == 0) {
                b->fn(argc, argv);
            }
        }
    }
    free(line);
    return EXIT_SUCCESS;
}