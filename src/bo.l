%{

#include "bo_internal.h"
#include "parser.h"

%}

%option 8bit
%option bison-bridge
  //%option debug
%option noinput
%option nounput
%option noyywrap
%option pointer
%option reentrant
%option warn

WHITESPACE    [ \t\r\n]
STRING_CHAR   [^"\\]|\\["\\nrt]|\\[0-9a-f]{2}|\\u[0-9A-Fa-f]{4}
VALUE_STRING  \"{STRING_CHAR}*\"
VALUE_HEX     [-+]?[0-9a-fA-F]+
VALUE_NUMBER  [-+]?[0-9]*\.?[0-9]*([eE][-+]?[0-9]+)?
WIDTH         1|2|4|8|16
NUMERIC_TYPE  [ihobfd]
ENDIAN        l|b
U_INT         [0-9]+

%%

{WHITESPACE}    {/* Ignored */}
{VALUE_NUMBER}|{VALUE_HEX} { yylval->string_v = yytext; return NUMBER; }
{VALUE_STRING} {
    const char* bad_data_loc = bo_unescape_string(yytext);
    if(bad_data_loc == NULL)
    {
        yylval->string_v = yytext;
        return STRING;
    }
    yylval->string_v = bad_data_loc;
    return BAD_DATA;
}

i{NUMERIC_TYPE}{WIDTH}{ENDIAN} { yylval->string_v = yytext+1; return INPUT_TYPE; }

o{NUMERIC_TYPE}{WIDTH}{ENDIAN}{U_INT} { yylval->string_v = yytext+1; return OUTPUT_TYPE; }

p{VALUE_STRING} {
    const char* bad_data_loc = bo_unescape_string(yytext + 1);
    if(bad_data_loc == NULL)
    {
        yylval->string_v = yytext;
        return PREFIX;
    }
    yylval->string_v = bad_data_loc;
    return BAD_DATA;
}

s{VALUE_STRING} {
    const char* bad_data_loc = bo_unescape_string(yytext + 1);
    if(bad_data_loc == NULL)
    {
        yylval->string_v = yytext;
        return SUFFIX;
    }
    yylval->string_v = bad_data_loc;
    return BAD_DATA;
}

. { yylval->string_v = yytext; return UNEXPECTED; }

%%


int bo_process_string(const char* input, char* output, int output_length, bo_config* config)
{
    yyscan_t scanner;
    if(yylex_init(&scanner) != 0)
    {
        config->on_error("Could not init scanner");
        return -1;
    }

    static const int work_buffer_size = 1600;
    yy_scan_string(input, scanner);
    bo_context context = bo_new_context(work_buffer_size, (uint8_t*)output, output_length, config);
    bool was_successful = yyparse(scanner, &context) == 0;
    bo_finish(&context);
    yylex_destroy(scanner);

    if(!was_successful)
    {
        config->on_error("Failed to parse");
        return -1;
    }

    return context.output_buffer.pos - context.output_buffer.start;
}

void yyerror(const void const *scanner __attribute__((unused)), bo_context* context, const char* const msg)
{
    context->config->on_error("%s", msg);
}
