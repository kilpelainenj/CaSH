#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int main(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while (1) {
        printf("cash> ");
        fflush(stdout);
        nread = getline(&line, &len, stdin);
        if (nread == -1) {
            break;
        } else if (nread == 0) {
            continue;
        }

        // getline leaves the end delimiter '\n' into the buffer
        if (line[nread-1] == '\n') {
            line[nread-1] = '\0';
        }
        // exit command
        if (strcmp(line, "exit") == 0) {
            break;
        }
    }
    free(line);
    return EXIT_SUCCESS;
}