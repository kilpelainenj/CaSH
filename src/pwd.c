#include "pwd.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

int do_pwd(int argc, char **argv) {
    // Don't unroll symlinks by default
    bool physical = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0) physical = true;
        else if (strcmp(argv[i], "-L") == 0) physical = false;
        else {
            fprintf(stderr, "pwd: invalid option '%s'\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    if (physical) {
        char *res = realpath(".", NULL);
        if (!res) {perror("pwd"); return EXIT_FAILURE;  }
        puts(res);
        free(res);
    } else {
        char *p  = getenv("PWD");
        if (!p) p = getcwd(NULL, 0);
        if (!p) { perror("pwd"); return EXIT_FAILURE; }
        puts(p);
    }
    return EXIT_SUCCESS;

}