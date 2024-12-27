%{
/* Headears - will be copied into scan.c */
#include <stdlib.h>
#include <math.h>

#include "parser.h"
#include "parse.h"

#define YYSTYPE GF_STYPE
%}

%option 8bit reentrant batch bison-bridge
%option warn nodefault
%option extra-type="parse_context *"
%option prefix="gf_"
%option noyywrap noinput nounput

%x quote

%%

 /* Definitions. */

[A-Za-z]([A-Za-z0-9_:]*[A-Za-z0-9_])? {
    yylval->string = gf_string_alloc(yytext, yyleng);
    if(!yylval->string) {
        gf_error(yyscanner, yyextra, "Unable to allocate string '%s'", yytext);
        yyterminate();
        return EOF;
    } else {
        return KEYWORD;
    }
}

[0-9]+(\.[0-9]*)?([eE][+-]?[0-9]+)? {
    char *fin = NULL;
    errno = 0;
    yylval->real = strtod(yytext, &fin);
    if( (yylval->real==0.0 || yylval->real==HUGE_VALF || yylval->real==HUGE_VALL) && errno==ERANGE) {
        gf_error(yyscanner, yyextra, "Invalid numeric constant '%s'", yytext);
        yyterminate();
    }
    return NUM;
}

#[^\n]*\n { yylineno++; /* ignore */ }

[=:;()\[\],+*/-] { return yytext[0]; }

[ \t] { /* ignore */ }

\n { yylineno++; }

\" {  BEGIN(quote); }

<quote>\" {
  BEGIN(INITIAL);
  yylval->string = gf_string_alloc(yytext, yyleng-1);
  if(!yylval->string) {
      gf_error(yyscanner, yyextra, "Unable to allocate string '%s'", yytext);
      yyterminate();
  } else {
      return STR;
  }
}

<quote>[\n\r] |
<quote><<EOF>> { gf_error(yyscanner, yyextra, "Unterminated quote");
 yyterminate();}

<quote>[^"\n\r]* { yymore(); }

. {
   if(yytext[0]&0x80)
       gf_error(yyscanner, yyextra, "invalid charactor '<unprintable>' (%d)", yytext[0]);
   else
       gf_error(yyscanner, yyextra, "invalid charactor '%c' (%d)", yytext[0], yytext[0]);
   yyterminate();
}

%%

/* Optional user code - will be copied into scan.c */
