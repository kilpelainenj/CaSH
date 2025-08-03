#ifndef COMMAND_H
#define COMMAND_H

#include "simple_command.h"

typedef struct {
    int capacity;
    int size;
    SimpleCommand **simple;

    char *outfile;
    char *infile;
    char *errfile;

    int background;
} Command;

void cmd_init(Command *cmd);
void cmd_clear(Command *cmd);

void cmd_insert_sc(Command *cmd, SimpleCommand *sc);

void cmd_print(const Command *cmd);
void cmd_execute(Command *cmd);
void cmd_prompt(void);




#endif