%{
#include <string.h>
#include "command.h"
#include "simple_command.h"
#include "shell.tab.h"
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


[^ \t\n][^ \t\n]* {
    yylval.string_val = strdup(yytext);
    return WORD;
    }

<<EOF>> {
    return 0; /* End of input */
    }

