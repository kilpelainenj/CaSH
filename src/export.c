#include "export.h"
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


int is_valid_name(const char *name) {
    if (!name || !*name) return 0;

    // First character must be alphanumeric or underscore
    if (!isalpha(*name) && *name != '_') return 0;

    // remaining chars must be alphanumeric or underscore
    for (int i = 1; name[i]; i++) {
        if (!isalnum(name[i]) && name[i] != '_') return 0;
    }

    return 1;
}

int do_export(int argc, char **argv) {
    if (argc == 1 || argv[1] == NULL) {
        extern char **environ;
        for (int i = 0; environ[i] != NULL; i++) {
            printf("declare -x %s\n", environ[i]);
        }
        return 0;

    }

    int ret = 0;

    for (int i = 1; i < argc && argv[i] != NULL; i++) {
        char *arg = strdup(argv[i]);
        if (!arg) {
            perror("export");
            return 1;
        }

        char *equals = strchr(arg, '=');
        char *name, *value;

        if (equals != NULL) {
            // Format: VAR=value
            *equals = '\0';
            name = arg;
            value = equals + 1;
        } else {
            // Format: VAR (export existing variable or create empty one)
            name = arg;
            value = getenv(name);
        }

        if (!is_valid_name(name)) {
            fprintf(stderr, "export: '%s': not a valid identifier\n", name);
            free(arg);
            ret = 1;
            continue;
        }

        if (value != NULL) {
            if (setenv(name, value, 1) != 0) {
                perror("export");
                free(arg);
                return 1;
            }
        } else {
            if (setenv(name, "", 1) != 0) {
                perror("export");
                free(arg);
                return 1;
            }
        }

        free(arg);
    }
    return ret;
}