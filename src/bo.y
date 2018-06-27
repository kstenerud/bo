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

%type <string_v> string STRING NUMBER UNEXPECTED BAD_DATA INPUT_TYPE OUTPUT_TYPE PREFIX SUFFIX

%token INPUT_TYPE OUTPUT_TYPE PREFIX SUFFIX STRING NUMBER UNEXPECTED BAD_DATA

%start begin

%%

begin: settings commands

settings:
    | settings setting

setting: input_type
       | output_type
       | prefix
       | suffix

commands:
    | commands command
    ;

command: string
       | number
       | input_type
       | bad_data
       ;

number: NUMBER           { if(!bo_on_number(context, $1)) return -1; }

input_type: INPUT_TYPE   { if(!bo_set_input_type(context, $1)) return -1; }
output_type: OUTPUT_TYPE { if(!bo_set_output_type(context, $1)) return -1; }
prefix: PREFIX           { if(!bo_set_prefix(context, $1)) return -1; }
suffix: SUFFIX           { if(!bo_set_suffix(context, $1)) return -1; }

bad_data: UNEXPECTED     { context->on_error("Unexpected token: %s", $1); return -1; }
        | BAD_DATA       { context->on_error("Bad encoding: %s", $1); return -1; }

string: STRING {
		char* str = (char*)$1;
		str[strlen(str)-1] = 0;
    if(!bo_on_string(context, str + 1)) return -1;
	}


%%

int yylex(YYSTYPE *, void *);
