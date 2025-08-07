#include "unset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

int is_valid_unset_name(const char *name) {
    if (!name || !*name) return 0;
    
    // First character must be letter or underscore
    if (!isalpha(*name) && *name != '_') return 0;
    
    // remaining characters must be alphanumeric or underscore
    for (int i = 1; name[i]; i++) {
        if (!isalnum(name[i]) && name[i] != '_') return 0;
    }
    
    return 1;
}

int do_unset(int argc, char **argv) {
    if (argc == 1 || argv[1] == NULL) {
        fprintf(stderr, "unset: usage: unset [-v] name [name ...]\n");
        return 1;
    }
    
    int ret = 0;
    int start_index = 1;
    
    // Handle optional -v flag (same behavior as default)
    if (argc > 1 && argv[1] != NULL && strcmp(argv[1], "-v") == 0) {
        start_index = 2;
        if (argc == 2) {
            fprintf(stderr, "unset: usage: unset [-v] name [name ...]\n");
            return 1;
        }
    }
    
    for (int i = start_index; i < argc && argv[i] != NULL; i++) {
        char *name = argv[i];
        
        if (strchr(name, '=') != NULL) {
            fprintf(stderr, "unset: `%s': cannot unset\n", name);
            ret = 1;
            continue;
        }
        
        if (!is_valid_unset_name(name)) {
            fprintf(stderr, "unset: `%s': not a valid identifier\n", name);
            ret = 1;
            continue;
        }
        
        if (unsetenv(name) != 0) {
            perror("unset");
            ret = 1;
            continue;
        }
        
        // unsetenv() doesn't fail if variable doesn't exist,
        // which matches bash behavior
    }
    
    return ret;
}