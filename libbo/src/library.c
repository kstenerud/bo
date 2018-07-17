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


#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bo_internal.h"
#include "library_version.h"


// -------------
// Configuration
// -------------

// The work buffer holds binary data that will be converted to whatever output format.
// For best results, keep this a multiple of 16.
#define WORK_BUFFER_SIZE 1600

// An overhead size of 32 ensures that for object sizes up to 128 bits,
// there's always room for 128 bits of zero filling at the end.
#define WORK_BUFFER_OVERHEAD_SIZE 32

// Use an overhead value large enough that the string printers won't blast past it in a single write.
#define OUTPUT_BUFFER_OVERHEAD_SIZE 200


#define BO_NATIVE_INT_ENDIANNESS (((__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) * BO_ENDIAN_LITTLE) + \
                                  ((__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) * BO_ENDIAN_BIG))
#define BO_NATIVE_FLOAT_ENDIANNESS (((__FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__) * BO_ENDIAN_LITTLE) + \
                                    ((__FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__) * BO_ENDIAN_BIG))


// ---------------
// Error Reporting
// ---------------

static void mark_error_condition(bo_context* context)
{
    context->is_error_condition = true;
    stop_parsing(context);
}

void bo_notify_error(bo_context* context, const char* fmt, ...)
{
    char buffer[500];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    buffer[sizeof(buffer)-1] = 0;
    va_end(args);

    mark_error_condition(context);
    context->on_error(context->user_data, buffer);
}



// ---------------
// String Printers
// ---------------

static inline void copy_swapped(uint8_t* dst, const uint8_t* src, int length)
{
	for(int i = 0; i < length; i++)
	{
		dst[i] = src[length - i - 1];
	}
}

/**
 * String printer.
 * Reads data from the source and writes to the destination.
 *
 * @param src Pointer to the source data.
 * @param dst Pointer to the destination buffer.
 * @param output_width in: Minimum bytes to print. out: Number of bytes written.
 * @return Number of bytes read.
 */
typedef int (*string_printer)(uint8_t* src, uint8_t* dst, int* output_width);

// Force the compiler to generate handler code for unaligned accesses.
#define DEFINE_SAFE_STRUCT(NAME, TYPE) typedef struct __attribute__((__packed__)) {TYPE contents;} NAME
DEFINE_SAFE_STRUCT(safe_uint_1,     uint8_t);
DEFINE_SAFE_STRUCT(safe_uint_2,     uint16_t);
DEFINE_SAFE_STRUCT(safe_uint_4,     uint32_t);
DEFINE_SAFE_STRUCT(safe_uint_8,     uint64_t);
DEFINE_SAFE_STRUCT(safe_int_1,      int8_t);
DEFINE_SAFE_STRUCT(safe_int_2,      int16_t);
DEFINE_SAFE_STRUCT(safe_int_4,      int32_t);
DEFINE_SAFE_STRUCT(safe_int_8,      int64_t);
DEFINE_SAFE_STRUCT(safe_int_16,     __int128);
DEFINE_SAFE_STRUCT(safe_float_4,    float);
DEFINE_SAFE_STRUCT(safe_float_8,    double);
DEFINE_SAFE_STRUCT(safe_float_16,   __float128);
DEFINE_SAFE_STRUCT(safe_decimal_8,  _Decimal64);
DEFINE_SAFE_STRUCT(safe_decimal_16, _Decimal128);

#define DEFINE_INT_STRING_PRINTER(NAMED_TYPE, REAL_TYPE, DATA_WIDTH, FORMAT) \
static int string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH (uint8_t* src, uint8_t* dst, int* output_width) \
{ \
	*output_width = sprintf((char*)dst, "%0*" #FORMAT, *output_width, ((safe_ ## REAL_TYPE ## _ ## DATA_WIDTH *)src)->contents); \
    return DATA_WIDTH; \
} \

#define DEFINE_INT_STRING_PRINTER_SWAPPED(NAMED_TYPE, REAL_TYPE, DATA_WIDTH, FORMAT) \
DEFINE_INT_STRING_PRINTER(NAMED_TYPE, REAL_TYPE, DATA_WIDTH, FORMAT) \
static int string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH ## _swapped (uint8_t* src, uint8_t* dst, int* output_width) \
{ \
	uint8_t buffer[DATA_WIDTH]; \
	copy_swapped(buffer, src, DATA_WIDTH); \
	return string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH (buffer, dst, output_width); \
}
DEFINE_INT_STRING_PRINTER(int, int, 1, d)
DEFINE_INT_STRING_PRINTER_SWAPPED(int, int, 2, d)
DEFINE_INT_STRING_PRINTER_SWAPPED(int, int, 4, d)
DEFINE_INT_STRING_PRINTER_SWAPPED(int, int, 8, ld)
// TODO: int-16
DEFINE_INT_STRING_PRINTER(hex, uint, 1, x)
DEFINE_INT_STRING_PRINTER_SWAPPED(hex, uint, 2, x)
DEFINE_INT_STRING_PRINTER_SWAPPED(hex, uint, 4, x)
DEFINE_INT_STRING_PRINTER_SWAPPED(hex, uint, 8, lx)
// TODO: hex-16
DEFINE_INT_STRING_PRINTER(octal, uint, 1, o)
DEFINE_INT_STRING_PRINTER_SWAPPED(octal, uint, 2, o)
DEFINE_INT_STRING_PRINTER_SWAPPED(octal, uint, 4, o)
DEFINE_INT_STRING_PRINTER_SWAPPED(octal, uint, 8, lo)
// TODO: octal-16

static int print_boolean_be(uint8_t* src, uint8_t* dst, int data_width, int text_width)
{
    int bit_width = data_width * 8;
    int total_width = (bit_width > text_width ? bit_width : text_width);
    int diff_width = total_width - bit_width;
    memset(dst, '0', bit_width);

    uint8_t* src_ptr = src;
    char* dst_ptr = (char*)dst + diff_width;

    for(int bits_remaining = bit_width;bits_remaining > 0;)
    {
        int value = *src_ptr;
        for(int i = 0; i < 8 && bits_remaining > 0; i++)
        {
            *dst_ptr++ = '0' + ((value >> (7-i)) & 1);
            bits_remaining--;
        }
        src_ptr++;
    }
    return total_width;
}

static int print_boolean_le(uint8_t* src, uint8_t* dst, int data_width, int text_width)
{
    int bit_width = data_width * 8;
    int total_width = (bit_width > text_width ? bit_width : text_width);
    int diff_width = total_width - bit_width;
    memset(dst, '0', bit_width);

    uint8_t* src_ptr = src;
    char* dst_ptr = (char*)dst + diff_width;

    for(int bits_remaining = bit_width;bits_remaining > 0;)
    {
        int value = *src_ptr;
        for(int i = 0; i < 8 && bits_remaining > 0; i++)
        {
            *dst_ptr++ = '0' + ((value >> i) & 1);
            bits_remaining--;
        }
        src_ptr++;
    }
    return total_width;
}
#define DEFINE_BOOLEAN_STRING_PRINTER(DATA_WIDTH) \
static int string_print_boolean_ ## DATA_WIDTH ## _be (uint8_t* src, uint8_t* dst, int* output_width) \
{ \
    *output_width = print_boolean_be(src, dst, DATA_WIDTH, *output_width); \
    return DATA_WIDTH; \
} \
static int string_print_boolean_ ## DATA_WIDTH ## _le (uint8_t* src, uint8_t* dst, int* output_width) \
{ \
    *output_width = print_boolean_le(src, dst, DATA_WIDTH, *output_width); \
    return DATA_WIDTH; \
}
DEFINE_BOOLEAN_STRING_PRINTER(1)
DEFINE_BOOLEAN_STRING_PRINTER(2)
DEFINE_BOOLEAN_STRING_PRINTER(4)
DEFINE_BOOLEAN_STRING_PRINTER(8)
DEFINE_BOOLEAN_STRING_PRINTER(16)

#define DEFINE_FLOAT_STRING_PRINTER(NAMED_TYPE, REAL_TYPE, DATA_WIDTH, FORMAT) \
static int string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH (uint8_t* src, uint8_t* dst, int* output_width) \
{ \
	*output_width = sprintf((char*)dst, "%.*" #FORMAT, *output_width, (double)((safe_ ## REAL_TYPE ## _ ## DATA_WIDTH *)src)->contents); \
    return DATA_WIDTH; \
} \
static int string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH ## _swapped (uint8_t* src, uint8_t* dst, int* output_width) \
{ \
	uint8_t buffer[DATA_WIDTH]; \
	copy_swapped(buffer, src, DATA_WIDTH); \
	return string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH (buffer, dst, output_width); \
}
// TODO: float-2
DEFINE_FLOAT_STRING_PRINTER(float, float, 4, f)
DEFINE_FLOAT_STRING_PRINTER(float, float, 8, f)
// TODO: float-16
// TODO: decimal-8
// TODO: decimal-16

#define DEFINE_BINARY_PRINTER(DATA_WIDTH) \
static int binary_print_ ## DATA_WIDTH (uint8_t* src, uint8_t* dst, int* output_width) \
{ \
    memcpy(dst, src, DATA_WIDTH); \
    *output_width = DATA_WIDTH; \
    return DATA_WIDTH; \
} \
static int binary_print_ ## DATA_WIDTH ## _swapped(uint8_t* src, uint8_t* dst, int* output_width) \
{ \
    copy_swapped(dst, src, DATA_WIDTH); \
    *output_width = DATA_WIDTH; \
    return DATA_WIDTH; \
}
DEFINE_BINARY_PRINTER(2)
DEFINE_BINARY_PRINTER(4)
DEFINE_BINARY_PRINTER(8)
DEFINE_BINARY_PRINTER(16)
static int binary_print_1(uint8_t* src, uint8_t* dst, int* output_width)
{
    *dst = *src;
    *output_width = 1;
    return 1;
}


static const char g_hex_values[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static int get_utf8_length(uint8_t ch)
{
    if((ch >> 5) == 0x06) return 2;
    if((ch >> 4) == 0x0e) return 3;
    if((ch >> 3) == 0x1e) return 4;
    return 0;
}

static int string_print_string(uint8_t* src, uint8_t* dst, int* output_width)
{
#define CASE_ESCAPE(VALUE, ESCAPED) \
    case VALUE: \
        dst[0] = '\\'; \
        dst[1] = ESCAPED; \
        *output_width = 2; \
        return 1

    uint8_t ch = *src;
    switch(ch)
    {
        CASE_ESCAPE(0x07, 'a');
        CASE_ESCAPE(0x08, 'b');
        CASE_ESCAPE(0x09, 't');
        CASE_ESCAPE(0x0a, 'n');
        CASE_ESCAPE(0x0b, 'v');
        CASE_ESCAPE(0x0c, 'f');
        CASE_ESCAPE(0x0d, 'r');
        CASE_ESCAPE('\\', '\\');
        CASE_ESCAPE('\"', '\"');
        CASE_ESCAPE('?', '?');
        default:
        {
            if(ch >= 0x20 && ch < 0x7f)
            {
                *dst = *src;
                *output_width = 1;
                return 1;
            }
            int utf8_length = get_utf8_length(ch);
            if(utf8_length > 0)
            {
                for(int i = 0; i < utf8_length; i++)
                {
                    *dst++ = *src++;
                }
                *output_width = utf8_length;
                return utf8_length;
            }
            dst[0] = '\\';
            dst[1] = 'x';
            dst[2] = g_hex_values[ch>>4];
            dst[3] = g_hex_values[ch&15];
            *output_width = 4;
            return 1;
        }
    }
}

bool matches_endianness(bo_context* context)
{
    return context->output.endianness == BO_NATIVE_INT_ENDIANNESS;
}

bool matches_float_endianness(bo_context* context)
{
    return context->output.endianness == BO_NATIVE_FLOAT_ENDIANNESS;
}

bool is_output_bigendian(bo_context* context)
{
    return context->output.endianness == BO_ENDIAN_BIG;
}

static string_printer get_string_printer(bo_context* context)
{
    switch(context->output.data_type)
    {
        case TYPE_INT:
        {
            switch(context->output.data_width)
            {
                case 1: return string_print_int_1;
                case 2: return matches_endianness(context) ? string_print_int_2 : string_print_int_2_swapped;
                case 4: return matches_endianness(context) ? string_print_int_4 : string_print_int_4_swapped;
                case 8: return matches_endianness(context) ? string_print_int_8 : string_print_int_8_swapped;
                case 16:
                    bo_notify_error(context, "TODO: INT 16 not implemented");
                    return NULL;
                default:
                    bo_notify_error(context, "%d: invalid data width", context->output.data_width);
                    return NULL;
            }
        }
        case TYPE_HEX:
        {
            switch(context->output.data_width)
            {
                case 1: return string_print_hex_1;
                case 2: return matches_endianness(context) ? string_print_hex_2 : string_print_hex_2_swapped;
                case 4: return matches_endianness(context) ? string_print_hex_4 : string_print_hex_4_swapped;
                case 8: return matches_endianness(context) ? string_print_hex_8 : string_print_hex_8_swapped;
                case 16:
                    bo_notify_error(context, "TODO: HEX 16 not implemented");
                    return NULL;
                default:
                    bo_notify_error(context, "%d: invalid data width", context->output.data_width);
                    return NULL;
            }
        }
        case TYPE_OCTAL:
        {
            switch(context->output.data_width)
            {
                case 1: return string_print_octal_1;
                case 2: return matches_endianness(context) ? string_print_octal_2 : string_print_octal_2_swapped;
                case 4: return matches_endianness(context) ? string_print_octal_4 : string_print_octal_4_swapped;
                case 8: return matches_endianness(context) ? string_print_octal_8 : string_print_octal_8_swapped;
                case 16:
                    bo_notify_error(context, "TODO: OCTAL 16 not implemented");
                    return NULL;
                default:
                    bo_notify_error(context, "%d: invalid data width", context->output.data_width);
                    return NULL;
            }
        }
        case TYPE_BOOLEAN:
        {
            switch(context->output.data_width)
            {
                case 1: return is_output_bigendian(context) ? string_print_boolean_1_be : string_print_boolean_1_le;
                case 2: return is_output_bigendian(context) ? string_print_boolean_2_be : string_print_boolean_2_le;
                case 4: return is_output_bigendian(context) ? string_print_boolean_4_be : string_print_boolean_4_le;
                case 8: return is_output_bigendian(context) ? string_print_boolean_8_be : string_print_boolean_8_le;
                case 16: return is_output_bigendian(context) ? string_print_boolean_16_be : string_print_boolean_16_le;
                default:
                    bo_notify_error(context, "%d: invalid data width", context->output.data_width);
                    return NULL;
            }
        }
        case TYPE_FLOAT:
        {
            switch(context->output.data_width)
            {
                case 2:
                    bo_notify_error(context, "TODO: FLOAT 2 not implemented");
                    return NULL;
                case 4: return matches_float_endianness(context) ? string_print_float_4 : string_print_float_4_swapped;
                case 8: return matches_float_endianness(context) ? string_print_float_8 : string_print_float_8_swapped;
                case 16:
                    bo_notify_error(context, "TODO: FLOAT 16 not implemented");
                    return NULL;
                default:
                    bo_notify_error(context, "%d: invalid data width", context->output.data_width);
                    return NULL;
            }
        }
        case TYPE_DECIMAL:
            bo_notify_error(context, "TODO: DECIMAL not implemented");
            return NULL;
        case TYPE_BINARY:
            switch(context->output.data_width)
            {
                case 1: return binary_print_1;
                case 2: return matches_endianness(context) ? binary_print_2 : binary_print_2_swapped;
                case 4: return matches_endianness(context) ? binary_print_4 : binary_print_4_swapped;
                case 8: return matches_endianness(context) ? binary_print_8 : binary_print_8_swapped;
                case 16: return matches_endianness(context) ? binary_print_16 : binary_print_16_swapped;
                default:
                    bo_notify_error(context, "%d: invalid data width", context->output.data_width);
                    return NULL;
            }
        case TYPE_STRING:
            return string_print_string;
        case TYPE_NONE:
            bo_notify_error(context, "Must set output data type before passing data");
            return NULL;
        default:
            bo_notify_error(context, "%d: Unknown data type", context->output.data_type);
            return NULL;
    }
}



// --------
// Internal
// --------

static inline bool is_nonempty_string(const char* const string)
{
    return string != NULL && *string != 0;
}

static inline bool check_can_input_numbers(bo_context* context, const uint8_t* string_value)
{
    if(context->input.data_type == TYPE_NONE || context->input.data_type == TYPE_STRING)
    {
        bo_notify_error(context, "%s: Must set input type to numeric before adding numbers", (const char*)string_value);
        return false;
    }
    return true;
}

static int trim_length_to_object_boundary(int length, int object_size_in_bytes)
{
    return length - (length % object_size_in_bytes);
}

static void clear_error_condition(bo_context* context)
{
    context->is_error_condition = false;
}



// ---------------
// Buffer Flushing
// ---------------

static void flush_buffer_to_output(bo_context* context, bo_buffer* buffer)
{
    if(!context->on_output(context->user_data, (char*)buffer_get_start(buffer), buffer_get_used(buffer)))
    {
        mark_error_condition(context);
    }
    buffer_clear(buffer);
}

static void flush_output_buffer(bo_context* context)
{
    LOG("Flush output buffer");
    flush_buffer_to_output(context, &context->output_buffer);
}

static void flush_work_buffer_binary(bo_context* context)
{
    flush_buffer_to_output(context, &context->work_buffer);
}

static void flush_work_buffer(bo_context* context, bool is_complete_flush)
{
    LOG("Flush work buffer");
    if(!buffer_is_initialized(&context->work_buffer) || buffer_is_empty(&context->work_buffer))
    {
        return;
    }

    if(context->output.data_type == TYPE_BINARY)
    {
        flush_work_buffer_binary(context);
        return;
    }

    string_printer string_print = get_string_printer(context);
    if(is_error_condition(context))
    {
        return;
    }

    bo_buffer* work_buffer = &context->work_buffer;
    bo_buffer* output_buffer = &context->output_buffer;

    int bytes_per_entry = context->output.data_width;
    int work_length = buffer_get_used(work_buffer);
    if(is_complete_flush)
    {
        // Ensure that the partial read at the end gets zeroes instead of random garbage.
        // WORK_BUFFER_OVERHEAD_SIZE makes sure this call doesn't run off the end of the buffer.
        memset(buffer_get_position(work_buffer), 0, 16);
    }
    else
    {
        work_length = trim_length_to_object_boundary(work_length, bytes_per_entry);
    }

    const bool has_prefix = is_nonempty_string(context->output.prefix);
    const bool has_suffix = is_nonempty_string(context->output.suffix);
    uint8_t* const start = buffer_get_start(work_buffer);
    uint8_t* const end = start + work_length;

    for(uint8_t* src = start; src < end;)
    {
        if(has_prefix)
        {
            buffer_append_string(output_buffer, context->output.prefix);
        }

        int output_width = context->output.text_width;
        int bytes_read = string_print(src, buffer_get_position(output_buffer), &output_width);
        if(output_width < 0)
        {
            bo_notify_error(context, "Error writing data");
            return;
        }
        buffer_use_space(output_buffer, output_width);

        if(has_suffix)
        {
            bool is_last_entry = src + bytes_per_entry >= end;
            if(!is_last_entry)
            {
                buffer_append_string(output_buffer, context->output.suffix);
            }
        }

        if(buffer_is_high_water(output_buffer))
        {
            flush_output_buffer(context);
            if(is_error_condition(context))
            {
                return;
            }
        }
        src += bytes_read;
    }
    buffer_clear(work_buffer);
}



// ------------------
// Binary Data Adders
// ------------------

static void add_bytes(bo_context* context, const uint8_t* ptr, int length)
{
    if(buffer_is_high_water(&context->work_buffer))
    {
        flush_work_buffer(context, false);
    }

    do
    {
        int copy_length = buffer_get_remaining(&context->work_buffer);
        if(copy_length > length)
        {
            copy_length = length;
        }
        buffer_append_bytes(&context->work_buffer, ptr, copy_length);
        if(buffer_is_high_water(&context->work_buffer))
        {
    	    flush_work_buffer(context, false);
        }
        if(is_error_condition(context))
	    {
	    	return;
	    }
        length -= copy_length;
        ptr += copy_length;
    } while(length > 0);
}

static void add_bytes_swapped(bo_context* context, const uint8_t* ptr, int length, const int width)
{
    if(buffer_is_high_water(&context->work_buffer))
    {
        flush_work_buffer(context, false);
    }

    const int remainder = length % width;
    const uint8_t* early_end = ptr + length - remainder;
    while(ptr < early_end)
    {
        copy_swapped(context->work_buffer.pos, ptr, width);
        buffer_use_space(&context->work_buffer, width);
        if(buffer_is_high_water(&context->work_buffer))
        {
            flush_work_buffer(context, false);
        }
        if(is_error_condition(context))
        {
            return;
        }
        ptr += width;
    }

    if(remainder > 0)
    {
        uint8_t bytes[width];
        memset(bytes, 0, sizeof(bytes));
        memcpy(bytes, ptr, remainder);
        copy_swapped(context->work_buffer.pos, bytes, width);
        buffer_use_space(&context->work_buffer, width);
    }
}

static void add_int(bo_context* context, uint64_t src_value)
{
    switch(context->input.data_width)
    {
        case WIDTH_1:
        {
            uint8_t value = (uint8_t)src_value;
            add_bytes(context, (uint8_t*)&value, sizeof(value));
            return;
        }
        case WIDTH_2:
        {
            uint16_t value = (uint16_t)src_value;
            if(context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
            {
            	uint8_t buff[sizeof(value)];
            	copy_swapped(buff, (uint8_t*)&value, sizeof(value));
	            add_bytes(context, buff, sizeof(value));
                return;
            }
            add_bytes(context, (uint8_t*)&value, sizeof(value));
            return;
        }
        case WIDTH_4:
        {
            uint32_t value = (uint32_t)src_value;
            if(context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
            {
            	uint8_t buff[sizeof(value)];
            	copy_swapped(buff, (uint8_t*)&value, sizeof(value));
	            add_bytes(context, buff, sizeof(value));
                return;
            }
            add_bytes(context, (uint8_t*)&value, sizeof(value));
            return;
        }
        case WIDTH_8:
        {
            uint64_t value = (uint64_t)src_value;
            if(context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
            {
            	uint8_t buff[sizeof(value)];
            	copy_swapped(buff, (uint8_t*)&value, sizeof(value));
	            add_bytes(context, buff, sizeof(value));
                return;
            }
            add_bytes(context, (uint8_t*)&value, sizeof(value));
            return;
        }
        case WIDTH_16:
		    bo_notify_error(context, "TODO: Int width 16 not implemented yet");
		    return;
        default:
		    bo_notify_error(context, "$d: Invalid int width", context->input.data_width);
		    return;
    }
}

static void add_float(bo_context* context, double src_value)
{
    switch(context->input.data_width)
    {
        case WIDTH_2:
		    bo_notify_error(context, "TODO: Float width 2 not implemented yet");
		    return;
        case WIDTH_4:
        {
            float value = (float)src_value;
            if(context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
            {
            	uint8_t buff[sizeof(value)];
            	copy_swapped(buff, (uint8_t*)&value, sizeof(value));
	            add_bytes(context, buff, sizeof(value));
                return;
            }
            add_bytes(context, (uint8_t*)&value, sizeof(value));
            return;
        }
        case WIDTH_8:
        {
            double value = (double)src_value;
            if(context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
            {
            	uint8_t buff[sizeof(value)];
            	copy_swapped(buff, (uint8_t*)&value, sizeof(value));
	            add_bytes(context, buff, sizeof(value));
                return;
            }
            add_bytes(context, (uint8_t*)&value, sizeof(value));
            return;
        }
        case WIDTH_16:
		    bo_notify_error(context, "TODO: Float width 16 not implemented yet");
		    return;
        default:
		    bo_notify_error(context, "%d: Invalid float width", context->input.data_width);
		    return;
    }
}



// ----------------
// Parser Callbacks
// ----------------

void bo_on_bytes(bo_context* context, uint8_t* data, int length)
{
    LOG("On bytes: %d", length);
    if(context->input.data_width > 1 && context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
    {
        add_bytes_swapped(context, data, length, context->input.data_width);
        return;
    }
    add_bytes(context, data, length);    
}

void bo_on_string(bo_context* context, const uint8_t* string_start, const uint8_t* string_end)
{
    LOG("On string [%s]", string_start);
    add_bytes(context, string_start, string_end - string_start);
}

void bo_on_number(bo_context* context, const uint8_t* string_value)
{
    LOG("On number [%s]", string_value);
    if(!check_can_input_numbers(context, string_value))
    {
        return;
    }

    switch(context->input.data_type)
    {
        case TYPE_FLOAT:
            add_float(context, strtod((char*)string_value, NULL));
            return;
        case TYPE_DECIMAL:
            bo_notify_error(context, "TODO: Unimplemented decimal type: %s", string_value);
            return;
        case TYPE_INT:
            add_int(context, strtoul((char*)string_value, NULL, 10));
            return;
        case TYPE_HEX:
            add_int(context, strtoul((char*)string_value, NULL, 16));
            return;
        case TYPE_OCTAL:
            add_int(context, strtoul((char*)string_value, NULL, 8));
            return;
        case TYPE_BOOLEAN:
            add_int(context, strtoul((char*)string_value, NULL, 2));
            return;
        default:
            bo_notify_error(context, "Unknown type %d for value [%s]", context->input.data_type, string_value);
            return;
    }
}

void bo_on_preset(bo_context* context, const uint8_t* string_value)
{
    LOG("Set preset [%s]", string_value);
    if(*string_value == 0)
    {
        bo_notify_error(context, "Missing preset value");
        return;
    }

    switch(*string_value)
    {
        case 's':
            bo_on_suffix(context, (uint8_t*)" ");
            break;
        case 'c':
            bo_on_suffix(context, (uint8_t*)", ");
            switch(context->output.data_type)
            {
                case TYPE_HEX:
                    bo_on_prefix(context, (uint8_t*)"0x");
                    break;
                case TYPE_OCTAL:
                    bo_on_prefix(context, (uint8_t*)"0");
                    break;
                default:
                    // Nothing to do
                    break;
            }
            break;
        default:
            bo_notify_error(context, "%s: Unknown prefix-suffix preset", string_value);
            return;
    }
}

void bo_on_prefix(bo_context* context, const uint8_t* prefix)
{
    LOG("Set prefix [%s]", prefix);
    if(context->output.prefix != NULL)
    {
        free((void*)context->output.prefix);
    }
    context->output.prefix = strdup((char*)prefix);
}

void bo_on_suffix(bo_context* context, const uint8_t* suffix)
{
    LOG("Set suffix [%s]", suffix);
    if(context->output.suffix != NULL)
    {
        free((void*)context->output.suffix);
    }
    context->output.suffix = strdup((char*)suffix);
}

void bo_on_input_type(bo_context* context, bo_data_type data_type, int data_width, bo_endianness endianness)
{
    LOG("Set input type %d, width %d, endianness %d", data_type, data_width, endianness);
    context->input.data_type = data_type;
    context->input.data_width = data_width;
    context->input.endianness = endianness;
}

void bo_on_output_type(bo_context* context, bo_data_type data_type, int data_width, bo_endianness endianness, int print_width)
{
    LOG("Set output type %d, width %d, endianness %d, print width %d", data_type, data_width, endianness, print_width);
    context->output.data_type = data_type;
    context->output.data_width = data_width;
    context->output.endianness = endianness;
    context->output.text_width = print_width;
}



// ----------
// Public API
// ----------

const char* bo_version()
{
    return BO_VERSION;
}

void* bo_new_context(void* user_data, output_callback on_output, error_callback on_error)
{
    LOG("New callback context");
    bo_context context =
    {
        .src_buffer = {0},
        .work_buffer = buffer_alloc(WORK_BUFFER_SIZE, WORK_BUFFER_OVERHEAD_SIZE),
        .output_buffer = buffer_alloc(WORK_BUFFER_SIZE * 10, OUTPUT_BUFFER_OVERHEAD_SIZE),
        .input =
        {
            .data_type = TYPE_NONE,
            .data_width = 0,
            .endianness = BO_ENDIAN_NONE,
        },
        .output =
        {
            .data_type = TYPE_NONE,
            .data_width = 0,
            .text_width = 0,
            .prefix = NULL,
            .suffix = NULL,
            .endianness = BO_ENDIAN_NONE,
        },
        .on_error = on_error,
        .on_output = on_output,
        .user_data = user_data,

        .data_segment_type = DATA_SEGMENT_STREAM,
        .is_at_end_of_input = false,
        .is_error_condition = false,
        .parse_should_continue = false,
        .is_spanning_string = false,
    };

    bo_context* heap_context = (bo_context*)malloc(sizeof(context));
    *heap_context = context;
    return heap_context;
}

bool bo_flush_and_destroy_context(void* void_context)
{
    LOG("Destroy context");
    bo_context* context = (bo_context*)void_context;
    clear_error_condition(context);
    flush_work_buffer(context, true);
    flush_output_buffer(context);
    bool is_successful = !is_error_condition(context);
    buffer_free(&context->work_buffer);
    buffer_free(&context->output_buffer);
    if(context->output.prefix != NULL)
    {
        free((void*)context->output.prefix);
    }
    if(context->output.suffix != NULL)
    {
        free((void*)context->output.suffix);
    }
    free((void*)context);
    return is_successful;
}
