%{

#include "bo_internal.h"
#include "parser.h"
#include <string.h>

#if YYBISON
union YYSTYPE;
int yylex(union YYSTYPE *, void *);
#endif

void yyerror(const void *scanner, void* context, const char *msg);

%}

%define api.pure full
%define parse.error verbose

%lex-param { void* scanner }

%parse-param { void* scanner }
%parse-param { bo_context* context }

%union {
    const char* string_v;
}

%type <string_v> string STRING NUMBER UNEXPECTED BAD_DATA INPUT_TYPE

%token INPUT_TYPE STRING NUMBER UNEXPECTED BAD_DATA

%start begin

%%

begin: input_type commands

commands:
    | commands command
    ;

command: string
       | number
       | input_type
       | bad_data
       ;

number: NUMBER         { if(!bo_on_number(context, $1)) return -1; }

input_type: INPUT_TYPE { if(!bo_set_data_type(context, $1)) return -1; }

bad_data: UNEXPECTED   { context->config->on_error("Unexpected token: %s", $1); return -1; }
        | BAD_DATA     { context->config->on_error("Bad encoding: %s", $1); return -1; }

string: STRING {
		char* str = (char*)$1;
		str[strlen(str)-1] = 0;
        if(!bo_on_string(context, str + 1)) return -1;
	}


%%

int yylex(YYSTYPE *, void *);
