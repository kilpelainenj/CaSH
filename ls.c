#include "ls.h"
#include <dirent.h>
#include <stdlib.h> 
#include <stdio.h>


int do_ls(int argc, char **argv) {
    const char *dir = (argc > 1 ? argv[1] : "."); 
    DIR *d = opendir(dir);
    if (!d) {
        perror(dir);
        return EXIT_FAILURE;
    }
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        // Skip hidden files
        if (entry->d_name[0] == '.') {
            continue;
        }
        printf("%s ", entry->d_name);
    }
    putchar('\n');
    closedir(d);
    return EXIT_SUCCESS;
}