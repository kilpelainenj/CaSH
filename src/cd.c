#include "cd.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int do_cd(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Missing target directory\n");
        return EXIT_FAILURE;
    }

    char* dir = argv[1];
    if (chdir(dir) != 0) {
        perror("cd");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}