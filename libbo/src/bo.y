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

#if YYBISON
union YYSTYPE;
int yylex(union YYSTYPE *, void *);
#endif

void yyerror(const void *scanner, void* context, const char *msg);

%}

%define parse.error verbose

%define api.pure full
%lex-param { void* scanner }
%parse-param { void* scanner } { bo_context* context }

%union {
    const char* string_v;
}

%token <string_v> STRING NUMBER INPUT_TYPE OUTPUT_TYPE INPUT_BINARY OUTPUT_BINARY PREFIX SUFFIX PREFIX_SUFFIX UNEXPECTED BAD_DATA

%start begin

%%

begin: commands

commands: | commands command

command: output_setting | input_setting | string | number | bad_data

output_setting: prefix | suffix | prefix_suffix | output_type | output_binary

input_setting: input_type | input_binary

input_type:    INPUT_TYPE    { bo_set_input_type(context, $1); if(context->is_error_condition) return -1; }
output_type:   OUTPUT_TYPE   { bo_set_output_type(context, $1); if(context->is_error_condition) return -1; }
input_binary:  INPUT_BINARY  { bo_set_input_type_binary(context, $1); if(context->is_error_condition) return -1; else return EARLY_EXIT_BINARY_MODE_MARKER; }
output_binary: OUTPUT_BINARY { bo_set_output_type_binary(context, $1); if(context->is_error_condition) return -1; }
prefix:        PREFIX        { bo_set_prefix(context, $1); if(context->is_error_condition) return -1; }
suffix:        SUFFIX        { bo_set_suffix(context, $1); if(context->is_error_condition) return -1; }
prefix_suffix: PREFIX_SUFFIX { bo_set_prefix_suffix(context, $1); if(context->is_error_condition) return -1; }
number:        NUMBER        { bo_on_number(context, $1); if(context->is_error_condition) return -1; }
string:        STRING        { bo_on_string(context, $1); if(context->is_error_condition) return -1; }
bad_data:      UNEXPECTED    { bo_notify_error(context, "Unexpected token: %s", $1); return -1; }
             | BAD_DATA      { bo_notify_error(context, "Bad encoding: %s", $1); return -1; }

%%

int yylex(YYSTYPE *, void *);
