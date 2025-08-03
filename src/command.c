#include "command.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

void cmd_init(Command *cmd) {
    cmd->capacity = 4;
    cmd->size = 0;
    cmd->simple = malloc(cmd->capacity * sizeof(*cmd->simple));
    if (!cmd->simple) {
        perror("malloc simple command array");
        return;
    }

    cmd->outfile = NULL;
    cmd->infile = NULL;
    cmd->errfile = NULL;

    cmd->background = 0;
}

void cmd_clear(Command *cmd) {
    for (int i = 0; i < cmd->size; i++) {
        sc_clear(cmd->simple[i]);
    }
    free(cmd->simple);
    cmd->simple = NULL;

    free(cmd->outfile);
    free(cmd->infile);
    free(cmd->errfile);

    cmd_init(cmd);
}

void cmd_insert_sc(Command *cmd, SimpleCommand *sc) {
    if (cmd->size +1 >= cmd->capacity) {
        cmd->capacity *= 2;
        cmd->simple = realloc(cmd->simple, (cmd->capacity + 1) * sizeof(*cmd->simple));
        if (!cmd->simple){
            perror("realloc simple command array");
            return;
        }
    }
    cmd->simple[cmd->size++] = sc;
}

void cmd_print(const Command *cmd) {
    printf("Command: %d stage(s)%s\n",
            cmd->size,
            cmd->background ? " [background]" : "");
    for(int i = 0; i < cmd->size; i++) {
        char **argv = cmd->simple[i]->arguments;
        printf(" stage %d:", i + 1);
        for(int j = 0; argv[j]; j++) {
            printf(" '%s'", argv[j]);
        }
        putchar('\n');
    }
    if (cmd->infile) {
        printf(" < %s\n", cmd->infile);
    }
    if (cmd->outfile) {
        printf(" > %s\n", cmd->outfile);
    }
    if (cmd->errfile) {
        printf(" 2> %s\n", cmd->errfile);
    }
}

void cmd_execute(Command *cmd) {
    if (!cmd || cmd->size == 0) {
        return;
    }

    pid_t pid = fork();
    for (int i = 0; i < cmd->size; i++) {
        if (pid < 0) {
            perror("fork");
            return exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process 
            execvp(cmd->simple[i]->arguments[0], cmd->simple[i]->arguments);
        } else { 
            if (!cmd->background) {
                waitpid(pid, NULL, 0);
            }
        }
    }

    // Just print the command for now
    cmd_print(cmd);
    
}

void cmd_prompt(void) {
}