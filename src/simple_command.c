#include "simple_command.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SC_INITIAL_CAP 4

static void sc_grow(SimpleCommand *sc);

void sc_init(SimpleCommand *sc) {
    sc->capacity = SC_INITIAL_CAP;
    sc->argc = 0;
    sc->arguments = malloc(sizeof(char *) * (sc->capacity +1)); /* +1 for NULL */
}

void sc_insert_arg(SimpleCommand *sc, const char *arg) {
    if (sc->argc == sc->capacity) {
        sc_grow(sc);
    }

    sc->arguments[sc->argc++] = strdup(arg); // Important to copy, because lexers typically just use a scratch buffer
    sc->arguments[sc->argc] = NULL; 

}

void sc_clear(SimpleCommand *sc) {
    for(int i = 0; i < sc->argc; i++){
        free(sc->arguments[i]);
    }

    free(sc->arguments);
    sc->arguments = NULL;
    sc->argc = sc->capacity = 0;
}

static void sc_grow(SimpleCommand *sc) {
    sc->capacity *= 2;
    sc->arguments = realloc(sc->arguments, sizeof(char *) * (sc->capacity +1));
}