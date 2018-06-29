%{

#include "bo_internal.h"
#include "parser.h"

static char* unescape_unquote_string(char* str);

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
    yylval->string_v = unescape_unquote_string(yytext);
    return (yylval->string_v == yytext + 1) ? STRING : BAD_DATA;
}

i{NUMERIC_TYPE}{WIDTH}{ENDIAN} { yylval->string_v = yytext+1; return INPUT_TYPE; }
o{NUMERIC_TYPE}{WIDTH}{ENDIAN}{U_INT} { yylval->string_v = yytext+1; return OUTPUT_TYPE; }
oB { return OUTPUT_BINARY; }

p{VALUE_STRING} {
    yylval->string_v = unescape_unquote_string(yytext + 1);
    return (yylval->string_v == yytext + 2) ? PREFIX : BAD_DATA;
}

s{VALUE_STRING} {
    yylval->string_v = unescape_unquote_string(yytext + 1);
    return (yylval->string_v == yytext + 2) ? SUFFIX : BAD_DATA;
}

. { yylval->string_v = yytext; return UNEXPECTED; }

%%


static char* unescape_unquote_string(char* str)
{
    char* maybe_bad_data_loc = bo_unescape_string(str);
    if(*maybe_bad_data_loc == '\0')
    {
        maybe_bad_data_loc[-1] = 0;
        return str + 1;
    }
    return maybe_bad_data_loc;
}

int bo_process_string(const char* input, char* output, int output_length, error_callback on_error)
{
    static const int work_buffer_size = 1600;
    bo_context context = bo_new_context(work_buffer_size, (uint8_t*)output, output_length, NULL, on_error);
    yyscan_t scanner;
    if(yylex_init(&scanner) != 0)
    {
        context.on_error("Could not init scanner");
        return -1;
    }

    YY_BUFFER_STATE buf = yy_scan_string(input, scanner);
    bool parse_was_successful = yyparse(scanner, &context) == 0;
    bool finish_was_successful = bo_finish(&context);
    yy_delete_buffer(buf, scanner);
    yylex_destroy(scanner);

    if(!(parse_was_successful && finish_was_successful))
    {
        context.on_error("Failed to parse");
        return -1;
    }

    return context.output_buffer.pos - context.output_buffer.start;
}

bool bo_process_stream(FILE* src, FILE* dst, error_callback on_error)
{
    static const int work_buffer_size = 1600;
    uint8_t* output[10000];
    bo_context context = bo_new_context(work_buffer_size, (uint8_t*)output, sizeof(output), dst, on_error);

    yyscan_t scanner;
    if(yylex_init(&scanner) != 0)
    {
        context.on_error("Could not init scanner");
        return false;
    }

    yyset_in(src, scanner);
    yyset_out(dst, scanner);
    bool parse_was_successful = yyparse(scanner, &context) == 0;
    bool finish_was_successful = bo_finish(&context);
    yylex_destroy(scanner);

    if(!(parse_was_successful && finish_was_successful))
    {
        context.on_error("Failed to parse");
        return false;
    }
    return true;
}

void yyerror(const void const *scanner __attribute__((unused)), bo_context* context, const char* const msg)
{
    context->on_error("%s", msg);
}
