%{
/*  Copyright (c) 2018 Karl Stenerud. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall remain in place
 * in this source code.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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
PREFIX_SUFFIX [cs]

%%

{WHITESPACE}    {/* Ignored */}
{VALUE_NUMBER}|{VALUE_HEX} { yylval->string_v = yytext; return NUMBER; }
{VALUE_STRING} {
    yylval->string_v = unescape_unquote_string(yytext);
    return (yylval->string_v == yytext + 1) ? STRING : BAD_DATA;
}

i{NUMERIC_TYPE}{WIDTH}{ENDIAN} { yylval->string_v = yytext+1; return INPUT_TYPE; }
o{NUMERIC_TYPE}{WIDTH}{ENDIAN}{U_INT} { yylval->string_v = yytext+1; return OUTPUT_TYPE; }
iB{WIDTH}{ENDIAN} { yylval->string_v = yytext+2; return INPUT_BINARY; }
oB{WIDTH}{ENDIAN} { yylval->string_v = yytext+2; return OUTPUT_BINARY; }

p{VALUE_STRING} {
    yylval->string_v = unescape_unquote_string(yytext + 1);
    return (yylval->string_v == yytext + 2) ? PREFIX : BAD_DATA;
}

s{VALUE_STRING} {
    yylval->string_v = unescape_unquote_string(yytext + 1);
    return (yylval->string_v == yytext + 2) ? SUFFIX : BAD_DATA;
}

P{PREFIX_SUFFIX} { yylval->string_v = yytext + 1; return PREFIX_SUFFIX; }

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

bool bo_process_string(void* void_context, const char* string)
{
    LOG("Process string [%s]", string);
    bo_context* context = (bo_context*)void_context;
    yyscan_t scanner;
    if(yylex_init(&scanner) != 0)
    {
        bo_notify_error(context, "Could not init scanner");
        return false;
    }

    YY_BUFFER_STATE buf = yy_scan_string(string, scanner);
    int parse_result = yyparse(scanner, context);
    yy_delete_buffer(buf, scanner);
    yylex_destroy(scanner);

    bool parse_was_successful = (parse_result == 0 || parse_result == EARLY_EXIT_BINARY_MODE_MARKER);
    if(!parse_was_successful)
    {
        bo_notify_error(context, "Failed to parse (result code %d)", parse_result);
        return false;
    }

    return true;
}

bool bo_process_stream(void* void_context, FILE* input_stream)
{
    bo_context* context = (bo_context*)void_context;

    LOG("Process stream as [%c]", context->input.data_type);
    if(context->input.data_type == TYPE_BINARY)
    {
        return bo_process_stream_as_binary(context, input_stream);
    }

    yyscan_t scanner;
    if(yylex_init(&scanner) != 0)
    {
        bo_notify_error(context, "Could not init scanner");
        return false;
    }

    yyset_in(input_stream, scanner);
    yyset_out(context->output_stream, scanner);
    int parse_result = yyparse(scanner, context);
    yylex_destroy(scanner);

    bool parse_was_successful = (parse_result == 0 || parse_result == EARLY_EXIT_BINARY_MODE_MARKER);
    if(!parse_was_successful)
    {
        bo_notify_error(context, "Failed to parse (result code %d)", parse_result);
        return false;
    }

    return true;
}

void yyerror(const void const *scanner __attribute__((unused)), bo_context* context, const char* const msg)
{
    bo_notify_error(context, "%s", msg);
}
