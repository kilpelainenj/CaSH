#include "command.h"
#include "builtins.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void cmd_init(Command* cmd)
{
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

void cmd_clear(Command* cmd)
{
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

void cmd_insert_sc(Command* cmd, SimpleCommand* sc)
{
    if (cmd->size + 1 >= cmd->capacity) {
        cmd->capacity *= 2;
        cmd->simple = realloc(cmd->simple, (cmd->capacity + 1) * sizeof(*cmd->simple));
        if (!cmd->simple) {
            perror("realloc simple command array");
            return;
        }
    }
    cmd->simple[cmd->size++] = sc;
}

void cmd_print(const Command* cmd)
{
    printf("Command: %d stage(s)%s\n", cmd->size, cmd->background ? " [background]" : "");
    for (int i = 0; i < cmd->size; i++) {
        char** argv = cmd->simple[i]->arguments;
        printf(" stage %d:", i + 1);
        for (int j = 0; argv[j]; j++) {
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

void cmd_execute(Command* cmd)
{
    int N = cmd->size;
    if (N == 0)
        return;

    if (N == 1) {
        extern const builtin_t builtins[];

        for (int j = 0; builtins[j].name; j++) {
            if (strcmp(cmd->simple[0]->arguments[0], builtins[j].name) == 0) {
                builtins[j].fn(cmd->simple[0]->argc, cmd->simple[0]->arguments);
                return;
            }
        }
    }

    int pipes[N - 1][2];
    for (int i = 0; i < N - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }

    for (int i = 0; i < N; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        }
        if (pid == 0) {
            // Child
            if (i > 0)
                dup2(pipes[i - 1][0], STDIN_FILENO);
            if (i < N - 1)
                dup2(pipes[i][1], STDOUT_FILENO);
            for (int j = 0; j < N - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            execvp(cmd->simple[i]->arguments[0], cmd->simple[i]->arguments);
            perror("execvp");
            return;
        }
        // Parent just goes to the next cmd
    }

    for (int i = 0; i < N - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    if (!cmd->background) {
        for (int i = 0; i < N; i++) {
            // Here we wait for ANY child process to terminate and do this for N times
            wait(NULL);
        }
    }
}
void cmd_prompt(void) {}