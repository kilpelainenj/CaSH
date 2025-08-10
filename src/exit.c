#include "exit.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int do_exit(int argc, char** argv)
{
    int code = EXIT_SUCCESS;

    if (argc > 1) {
        char* end;
        long v = strtol(argv[1], &end, 10);

        if (*end != '\0') {
            fprintf(stderr, "exit: numeric argument required: %s\n", argv[1]);
            code = EXIT_FAILURE;
        } else {
            code = (int)v;
        }
    }
    exit(code);
}