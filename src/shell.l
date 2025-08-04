%{
#include <string.h>
#include "command.h"
#include "simple_command.h"
#include "shell.tab.h"
#include <stdlib.h>
%}

%%

\n {
    return NEWLINE;
    }

[ \t] {
    /* Discard spaces and tabs */
    }


"|" {
    return PIPE;
    }

"&" {
    return AMPERSAND;
    }

">>" {
    return GREATGREAT;
    }

"<" {
    return LESS;
    }

">" {
    return GREAT;
    }

">&" {
    return GREATAMPERSAND;
    }

\$[a-zA-Z_][a-zA-Z0-9_]* {
    /* $VAR format */
    char *var_name = yytext + 1; /* Skip the $ */
    char *value = getenv(var_name);
    yylval.string_val = strdup(value ? value : "");
    return WORD;
}

\$\{[a-zA-Z_][a-zA-Z0-9_]*\} {
    /* ${VAR} format*/
    char var_name[256];
    strncpy(var_name, yytext + 2, strlen(yytext) - 3); // skip $ and {}
    var_name[strlen(yytext) - 3] = '\0'; // Add terminating character
    char *value = getenv(var_name);
    yylval.string_val = strdup(value ? value : "");
    return WORD;
}


[^ \t\n][^ \t\n]* {
    yylval.string_val = strdup(yytext);
    return WORD;
    }

<<EOF>> {
    return 0; /* End of input */
    }

