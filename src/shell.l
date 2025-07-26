%{
#include <string.h>
#include "y.tab.h"
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

">&" {
    return GREATAMPERSAND
    }


[^ \t\n][^ \t\n]* {
    yylval.string_val = strdup(yytext);
    return WORD;
    }

