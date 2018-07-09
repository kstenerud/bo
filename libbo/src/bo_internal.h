//  Copyright (c) 2018 Karl Stenerud. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall remain in place
// in this source code.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//


#ifndef bo_internal_H
#define bo_internal_H
#ifdef __cplusplus
extern "C" {
#endif


// #define BO_ENABLE_LOGGING 1


#include <stdbool.h>
#include <stdint.h>

#include "bo/bo.h"


#if BO_ENABLE_LOGGING
#define LOG(...) do {fprintf(stdout, "LOG: ");fprintf(stdout, __VA_ARGS__);fprintf(stdout, "\n");fflush(stdout);} while(0)
#else
#define LOG(FMT, ...)
#endif


typedef enum
{
    TYPE_NONE    = 0,
    TYPE_BINARY  = 'B',
    TYPE_INT     = 'i',
    TYPE_HEX     = 'h',
    TYPE_OCTAL   = 'o',
    TYPE_BOOLEAN = 'b',
    TYPE_FLOAT   = 'f',
    TYPE_DECIMAL = 'd',
    TYPE_STRING  = 's',
} bo_data_type;

typedef enum
{
    BO_ENDIAN_NONE = 0,
    BO_ENDIAN_LITTLE = 'l',
    BO_ENDIAN_BIG = 'b',
} bo_endianness;

typedef enum
{
    WIDTH_1  =  1,
    WIDTH_2  =  2,
    WIDTH_4  =  4,
    WIDTH_8  =  8,
    WIDTH_16 = 16,
} bo_data_width;

typedef struct
{
    uint8_t* start;
    uint8_t* end;
    uint8_t* pos;
    uint8_t* high_water;
} bo_buffer;

typedef struct
{
    bo_buffer work_buffer;
    bo_buffer output_buffer;
    struct
    {
        bo_data_type data_type;
        bo_data_width data_width;
        bo_endianness endianness;
    } input;
    struct
    {
        bo_data_type data_type;
        int data_width;
        int text_width;
        const char* prefix;
        const char* suffix;
        bo_endianness endianness;
    } output;
    error_callback on_error;
    output_callback on_output;
    void* user_data;

    char* parse_position;
    bool is_last_data_segment;
    bool is_at_end_of_input;
    bool is_error_condition;
    bool parse_should_continue;
} bo_context;


void bo_on_bytes(bo_context* context, uint8_t* data, int length);
void bo_on_string(bo_context* context, const char* string);
void bo_on_number(bo_context* context, const char* string_value);

void bo_on_preset(bo_context* context, const char* string_value);
void bo_on_prefix(bo_context* context, const char* prefix);
void bo_on_suffix(bo_context* context, const char* suffix);
void bo_on_input_type(bo_context* context, bo_data_type data_type, int data_width, bo_endianness endianness);
void bo_on_output_type(bo_context* context, bo_data_type data_type, int data_width, bo_endianness endianness, int print_width);

void bo_notify_error(bo_context* context, char* fmt, ...);


static inline bool should_continue_parsing(bo_context* context)
{
    return context->parse_should_continue;
}

static inline bool is_error_condition(bo_context* context)
{
    return context->is_error_condition;
}

#ifdef __cplusplus
}
#endif
#endif // bo_internal_H
