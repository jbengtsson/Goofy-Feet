
#include <stdio.h>

#include "parser.h"

#define YYSTYPE         GF_STYPE
extern "C" {
  // the generated headers wrap some parts in extern "C" blocks, but not all...
#include "parse.h"
#include "scan.h"
}

int main(int argc, char *argv[])
{
  FILE *in = stdin;
  if(argc>1) {
    in = fopen(argv[1], "r");
    if(!in) {
      fprintf(stderr, "Failed to open %s\n", argv[1]);
      return 2;
    }
  }

  parse_context ctxt;

  yyscan_t scanner;

  gf_lex_init_extra(&ctxt, &scanner);

  gf__switch_to_buffer(gf__create_buffer(in, 1024, scanner), scanner);

  int ret = 0;

  while(1) {
    YYSTYPE lval;
    int tok = gf_lex(&lval, scanner);
    if(tok==0)
      break;

    switch(tok) {
    case NUM:
      printf("Number: %g\n", lval.real);
      break;
    case STR:
      printf("String: \"%s\"\n", lval.string->str.c_str());
      delete lval.string;
      break;
    case KEYWORD:
      printf("KW: %s\n", lval.string->str.c_str());
      delete lval.string;
      break;
    case COMMENT:
      break;
    case NEG:
      printf("NEG\n");
      break;
      //[=:;()\[\],+*/-]
    case '=':
    case ':':
    case ';':
    case '(':
    case ')':
    case '[':
    case ']':
    case ',':
    case '+':
    case '-':
    case '*':
    case '/':
      printf("Literal: '%c'\n", tok);
      break;

    default:
      printf("Unknown token type '%c' (%d)\n", tok, tok);
      ret = 1;
    }
    if(ret)
      break;
  }
  printf("EOF\n");

  gf_lex_destroy(scanner);

  if(ctxt.last_error.size()) {
    ret = 1;
    fprintf(stderr, "Error: %s\n", ctxt.last_error.c_str());
  }

  fclose(in);

  return ret;
}
