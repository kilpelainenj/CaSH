#ifndef COMMAND_H
#define COMMAND_H

#include "simple_command.h"

typedef struct {
    int capacity;
    // Tells how many simple commands are in the command
    int size;
    // Array of simple commands
    SimpleCommand** simple;

    char* outfile;
    char* infile;
    char* errfile;

    // A flag to tell if the command should run on the background
    int background;
} Command;

void cmd_init(Command* cmd);
void cmd_clear(Command* cmd);

void cmd_insert_sc(Command* cmd, SimpleCommand* sc);

void cmd_print(const Command* cmd);
void cmd_execute(Command* cmd);
void cmd_prompt(void);

#endif