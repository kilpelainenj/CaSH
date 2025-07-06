#include "cat.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int do_cat(int argc, char **argv) {
    if (argc == 1) {
        fprintf(stderr, "cat: Missing operand\n");
        return EXIT_FAILURE;
    }
    const char *filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror(filename);
        return EXIT_FAILURE;
    }
    char buf[4096];
    ssize_t nread;
    while ((nread = read(fd, buf, sizeof(buf))) > 0) {
        ssize_t nwritten = write(STDOUT_FILENO, buf, nread);
        // Check for partial writes
        if (nwritten != nread) {
            perror("write");
            close(fd);
            return EXIT_FAILURE;
        }
    }
    // Error message in case the user gives bad input
    if (nread == -1) {
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }

    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}