
%{
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

#undef yyerror
static inline
void yyerror (yacc_arg arg, const char* msg)
{
    gf_error(arg.scanner, arg.ctxt, "%s", msg);
}
%}

%define api.prefix {gf_}
%define api.pure full
%define parse.trace

%parse-param {yacc_arg parg}
%lex-param {void *scanner}

%union { string_t *string; }
%destructor { gf_string_cleanup($$); $$ = NULL; } <string>

%token <string> KEYWORD STR

%printer { gf_string_debug(yyoutput, $$); } KEYWORD STR;

%union { double real; }
%token <real> NUM

%printer { fprintf(yyoutput, "%g", $$); } NUM;

%union { expr_t *exprv; }
%destructor { gf_expr_cleanup($$); } <exprv>
%type <exprv> expr

%printer { gf_expr_debug(yyoutput, $$); } expr;

%union { vector_t *vector; }
%destructor { gf_vector_cleanup($$); } <vector>
%type <vector> expr_list vector

%printer { fprintf(yyoutput, "%p", (void *)$$); } expr_list vector;

%union { kvlist_t *kvlist; }
%destructor { gf_kvlist_cleanup($$); } <kvlist>
%type <kvlist> properties

%printer { fprintf(yyoutput, "%p", (void *)$$); } properties;

%union { strlist_t *strlist; }
%destructor { gf_strlist_cleanup($$); } <strlist>
%type <strlist> line_list

%printer { fprintf(yyoutput, "%p", (void *)$$); } line_list;

%union { kv_t kv; }
%destructor { free($$.key); $$.key = NULL;gf_expr_cleanup($$.value); } <kv>
%type <kv> property

%token COMMENT

%left '-' '+'
%left '*' '/'
%precedence NEG

%start file

%{
#include "scan.h"

#define PCLR(X) (X) = NULL
#define PERR(X) if(!X) YYERROR;

#define scanner (yyscan_t)(parg.scanner)
#define ctxt parg.ctxt
%}

%%

file : %empty
     | entry file

entry : assignment
      | element
      | line
      | func
      | command

assignment : KEYWORD '=' expr ';' { gf_assign(ctxt, $1, $3); PCLR($1); }

element : KEYWORD ':' KEYWORD properties ';' { gf_add_element(ctxt, $1, $3, $4); PCLR($1); PCLR($3); PCLR($4); }

line : KEYWORD ':' KEYWORD '=' '(' line_list ')' ';' { gf_add_line(ctxt, $1, $3, $6); PCLR($1); PCLR($3); PCLR($6); }

func : KEYWORD '(' expr ')' ';' { gf_call1(ctxt, $1, $3); PCLR($1); PCLR($3); }

command : KEYWORD ';' { gf_command(ctxt, $1); }

properties : %empty                   { $$ = NULL; }
           | ',' property properties  { $$ = gf_append_kv(ctxt, $3, &$2); PCLR($3); PERR($$); }

property : KEYWORD '=' expr { $$.key = $1; $$.value = $3; PCLR($1); PCLR($3); }

line_list : %empty             { $$ = NULL; }
          | expr               { $$ = gf_append_expr(ctxt, NULL, $1); PERR($$); }
          | expr ',' line_list { $$ = gf_append_expr(ctxt, $3, $1); PERR($$); }

expr : NUM                  { $$ = gf_add_value(ctxt, gf_expr_number, $1); }
     | vector               { $$ = gf_add_value(ctxt, gf_expr_vector, $1); PCLR($1); PERR($$); }
     | STR                  { $$ = gf_add_value(ctxt, gf_expr_string, $1); PCLR($1); PERR($$); }
     | KEYWORD              { $$ = gf_add_value(ctxt, gf_expr_var, $1); PCLR($1); PERR($$); }
     | expr '+' expr        { expr_t *A[2] = {$1,$3}; $$ = gf_add_op(ctxt, gf_string_alloc("+",1), 2, A); PCLR($1); PCLR($3); PERR($$); }
     | expr '-' expr        { expr_t *A[2] = {$1,$3}; $$ = gf_add_op(ctxt, gf_string_alloc("-",1), 2, A); PCLR($1); PCLR($3); PERR($$); }
     | expr '*' expr        { expr_t *A[2] = {$1,$3}; $$ = gf_add_op(ctxt, gf_string_alloc("*",1), 2, A); PCLR($1); PCLR($3); PERR($$); }
     | expr '/' expr        { expr_t *A[2] = {$1,$3}; $$ = gf_add_op(ctxt, gf_string_alloc("/",1), 2, A); PCLR($1); PCLR($3); PERR($$); }
     | '-' expr %prec NEG   { expr_t *A[1] = {$2}; $$ = gf_add_op(ctxt, gf_string_alloc("-",1), 1, A); PCLR($2); PERR($$); }
     | '(' expr ')'         { $$ = $2; PCLR($2); PERR($$); }
     | KEYWORD '(' expr ')' { expr_t *A[1] = {$3}; $$ = gf_add_op(ctxt, $1, 1, A); PCLR($1); PCLR($3); PERR($$); }

expr_list : %empty             { $$ = NULL; }
          | expr               { $$ = gf_append_vector(ctxt, NULL, $1); PERR($$); }
          | expr ',' expr_list { $$ = gf_append_vector(ctxt, $3, $1); PERR($$); }

vector : '[' expr_list ']'  { $$ = $2; }
