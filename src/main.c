#include "main.h"
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include "command.h"
#include "simple_command.h"
#include "exit.h"
#include "pwd.h"
#include "export.h"
#include "unset.h"

//extern int do_exit(int, char**);
//extern int do_cd(int, char**);
extern int do_cd(int, char**);
extern int print_ascii(void);
extern int do_exit(int, char**);
extern int do_pwd(int, char**);
extern int do_export(int, char**);
extern int do_unset(int, char**);



// These are the built-ins that have to run in the main process,
// because they modify the shell state
builtin_t builtins[] = {
    { "exit", do_exit},
    { "export", do_export},
    { "unset", do_unset},
    { "pwd", do_pwd},
    { "cd", do_cd},
    { NULL, NULL}
};



int main(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    print_ascii();

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

        // Prepare input for parser (ensure it ends with newline)
        char *input_line = malloc(strlen(line) + 2);
        strcpy(input_line, line);
        
        // Check if the line already ends with newline
        if (line[nread-1] == '\n') {
            line[nread-1] = '\0';
            // Don't add another newline since it was already there
            input_line[strlen(input_line)] = '\0'; 
        } else {
            strcat(input_line, "\n");
        }
        
        // For debugging purposes
        /*
        fprintf(stderr, "Input to parser: '%s' (length: %zu)\n", input_line, strlen(input_line));
        for (size_t i = 0; i < strlen(input_line); i++) {
            fprintf(stderr, "char[%zu] = '%c' (%d)\n", i, input_line[i], input_line[i]);
        }
        */        
        // Create a temp file for the parser
        FILE *temp_file = tmpfile();
        if (!temp_file) {
            perror("tmpfile");
            free(input_line);
            continue;
        }
        
        // Write the input to the temp file and set it as the input for the PARSER
        fwrite(input_line, 1, strlen(input_line), temp_file);
        rewind(temp_file);
        
        
        extern FILE *yyin;
        extern int yyparse(void);
        yyin = temp_file;
        
        
        yyparse();
        
        fclose(temp_file);
        free(input_line);
        
    }
    free(line);
    return EXIT_SUCCESS;
}