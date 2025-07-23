#ifndef SIMPLE_COMMAND_H
#define SIMPLE_COMMAND_H

typedef struct {
    int capacity;
    int argc;
    char **arguments;
} SimpleCommand;

void sc_init(SimpleCommand *sc);
void sc_clear(SimpleCommand *sc);

void sc_insert_arg(SimpleCommand *sc, const char *arg);

#endif