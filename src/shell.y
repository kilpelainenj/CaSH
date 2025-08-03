%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
#include "simple_command.h"
extern int yylex(void);
extern void yyerror(const char *s);

Command *current_command = NULL;
%}

%union {
    char *string_val;
    Command *command_val;
    SimpleCommand *simple_command_val;
}

%token <string_val> WORD
%token NEWLINE
%token PIPE
%token AMPERSAND
%token GREATGREAT
%token LESS
%token GREAT
%token GREATAMPERSAND
%token END

%type <command_val> command pipeline simple_command_list
%type <simple_command_val> simple_command

%%

program
    : command_list
    ;

command_list
    : command NEWLINE {
        if (current_command) {
            cmd_execute(current_command);
            cmd_clear(current_command);
            free(current_command);
            current_command = NULL;
        }
    }
    | command_list command NEWLINE {
        if (current_command) {
            cmd_execute(current_command);
            cmd_clear(current_command);
            free(current_command);
            current_command = NULL;
        }
    }
    | command NEWLINE END {
        if (current_command) {
            cmd_execute(current_command);
            cmd_clear(current_command);
            free(current_command);
            current_command = NULL;
        }
    }
    ;

command
    : pipeline {
        current_command = $1;
    }
    | command GREAT WORD {
        if (current_command) {
            current_command->outfile = strdup($3);
        }
    }
    | command LESS WORD {
        if (current_command) {
            current_command->infile = strdup($3);
        }
    }
    | command GREATGREAT WORD {
        if (current_command) {
            current_command->outfile = strdup($3);
        }
    }
    | command GREATAMPERSAND WORD {
        if (current_command) {
            current_command->errfile = strdup($3);
        }
    }
    | command AMPERSAND {
        if (current_command) {
            current_command->background = 1;
        }
    }
    ;

pipeline
    : simple_command_list {
        $$ = $1;
    }
    | pipeline PIPE simple_command_list {
        cmd_insert_sc($1, $3->simple[0]);
        free($3);
        $$ = $1;
    }
    ;

simple_command_list
    : simple_command {
        Command *cmd = malloc(sizeof(Command));
        cmd_init(cmd);
        cmd_insert_sc(cmd, $1);
        $$ = cmd;
    }
    ;

simple_command
    : WORD {
        SimpleCommand *sc = malloc(sizeof(SimpleCommand));
        sc_init(sc);
        sc_insert_arg(sc, $1);
        $$ = sc;
    }
    | simple_command WORD {
        sc_insert_arg($1, $2);
        $$ = $1;
    }
    ;

%%

// Error handling
void yyerror(const char *s) {
    fprintf(stderr, "Parse error: %s\n", s);
    extern char *yytext;
    extern int yychar;
    fprintf(stderr, "Last token: '%s' (token type: %d)\n", yytext ? yytext : "NULL", yychar);
}
// Only one file as input
// One could use a loop to read multiple files, worth doing idk?
int yywrap(void) {
    return 1;
}
