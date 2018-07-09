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


#include <stdio.h>
#include <memory.h>
#include <stdarg.h>
#include <errno.h>

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
#define OUTPUT_BUFFER_OVERHEAD_SIZE 100



// ---------------
// Error Reporting
// ---------------

static void mark_error_condition(bo_context* context)
{
    context->is_error_condition = true;
    context->parse_should_continue = false;
}

void bo_notify_error(bo_context* context, char* fmt, ...)
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

static void bo_notify_posix_error(bo_context* context, char* fmt, ...)
{
    int error_num = errno;
    char buffer[500];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    buffer[sizeof(buffer)-1] = 0;
    va_end(args);

    int length = strlen(buffer);
    int remaining = sizeof(buffer) - length;
    strncpy(buffer + length, ": ", 2);
    buffer[sizeof(buffer)-1] = 0;

    length = strlen(buffer);
    remaining = sizeof(buffer) - length;
    strerror_r(error_num, buffer + length, remaining);
    buffer[sizeof(buffer)-1] = 0;

    mark_error_condition(context);
    context->on_error(context->user_data, buffer);
}



// ---------------
// String Printers
// ---------------

static inline void copy_swapped(uint8_t* dst, uint8_t* src, int length)
{
	for(int i = 0; i < length; i++)
	{
		dst[i] = src[length - i - 1];
	}
}

typedef int (*string_printer)(uint8_t* src, char* dst, int text_width, bo_endianness endianness);

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
static int string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH ## _internal (uint8_t* src, char* dst, int text_width) \
{ \
	return sprintf(dst, "%0*" #FORMAT, text_width, ((safe_ ## REAL_TYPE ## _ ## DATA_WIDTH *)src)->contents); \
} \
static int string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH (uint8_t* src, char* dst, int text_width, bo_endianness endianness) \
{ \
	if(DATA_WIDTH == 1 || endianness == BO_NATIVE_INT_ENDIANNESS) \
	{ \
		return string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH ## _internal (src, dst, text_width); \
	} \
\
	uint8_t buffer[DATA_WIDTH]; \
	copy_swapped(buffer, src, DATA_WIDTH); \
	return string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH ## _internal (buffer, dst, text_width); \
}
DEFINE_INT_STRING_PRINTER(int, int, 1, d)
DEFINE_INT_STRING_PRINTER(int, int, 2, d)
DEFINE_INT_STRING_PRINTER(int, int, 4, d)
DEFINE_INT_STRING_PRINTER(int, int, 8, ld)
// TODO: int-16
DEFINE_INT_STRING_PRINTER(hex, uint, 1, x)
DEFINE_INT_STRING_PRINTER(hex, uint, 2, x)
DEFINE_INT_STRING_PRINTER(hex, uint, 4, x)
DEFINE_INT_STRING_PRINTER(hex, uint, 8, lx)
// TODO: hex-16
DEFINE_INT_STRING_PRINTER(octal, uint, 1, o)
DEFINE_INT_STRING_PRINTER(octal, uint, 2, o)
DEFINE_INT_STRING_PRINTER(octal, uint, 4, o)
DEFINE_INT_STRING_PRINTER(octal, uint, 8, lo)
// TODO: octal-16

#define DEFINE_FLOAT_STRING_PRINTER(NAMED_TYPE, REAL_TYPE, DATA_WIDTH, FORMAT) \
static int string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH ## _internal (uint8_t* src, char* dst, int text_width) \
{ \
	return sprintf(dst, "%.*" #FORMAT, text_width, (double)((safe_ ## REAL_TYPE ## _ ## DATA_WIDTH *)src)->contents); \
} \
static int string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH (uint8_t* src, char* dst, int text_width, bo_endianness endianness) \
{ \
	if(endianness == BO_NATIVE_FLOAT_ENDIANNESS) \
	{ \
		return string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH ## _internal (src, dst, text_width); \
	} \
\
	uint8_t buffer[DATA_WIDTH]; \
	copy_swapped(buffer, src, DATA_WIDTH); \
	return string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH ## _internal (buffer, dst, text_width); \
}
// TODO: float-2
DEFINE_FLOAT_STRING_PRINTER(float, float, 4, f)
DEFINE_FLOAT_STRING_PRINTER(float, float, 8, f)
// TODO: float-16
// TODO: decimal-8
// TODO: decimal-16

#define DEFINE_BINARY_PRINTER(DATA_WIDTH) \
static int binary_print_ ## DATA_WIDTH ## _le(uint8_t* src, char* dst, int text_width, bo_endianness endianness) \
{ \
    if(BO_NATIVE_INT_ENDIANNESS == BO_ENDIAN_LITTLE) \
    { \
        memcpy(dst, src, DATA_WIDTH); \
    } \
    else \
    { \
        copy_swapped(dst, src, DATA_WIDTH); \
    } \
    return DATA_WIDTH; \
} \
static int binary_print_ ## DATA_WIDTH ## _be(uint8_t* src, char* dst, int text_width, bo_endianness endianness) \
{ \
    if(BO_NATIVE_INT_ENDIANNESS == BO_ENDIAN_BIG) \
    { \
        memcpy(dst, src, DATA_WIDTH); \
    } \
    else \
    { \
        copy_swapped(dst, src, DATA_WIDTH); \
    } \
    return DATA_WIDTH; \
}
DEFINE_BINARY_PRINTER(2)
DEFINE_BINARY_PRINTER(4)
DEFINE_BINARY_PRINTER(8)
DEFINE_BINARY_PRINTER(16)
static int binary_print_1(uint8_t* src, char* dst, int text_width, bo_endianness endianness)
{
    *dst = *src;
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
                case 2: return string_print_int_2;
                case 4: return string_print_int_4;
                case 8: return string_print_int_8;
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
                case 2: return string_print_hex_2;
                case 4: return string_print_hex_4;
                case 8: return string_print_hex_8;
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
                case 2: return string_print_octal_2;
                case 4: return string_print_octal_4;
                case 8: return string_print_octal_8;
                case 16:
                    bo_notify_error(context, "TODO: OCTAL 16 not implemented");
                    return NULL;
                default:
                    bo_notify_error(context, "%d: invalid data width", context->output.data_width);
                    return NULL;
            }
        }
        case TYPE_BOOLEAN:
            bo_notify_error(context, "TODO: BOOLEAN not implemented");
            return NULL;
        case TYPE_FLOAT:
        {
            switch(context->output.data_width)
            {
                case 2:
                    bo_notify_error(context, "TODO: FLOAT 2 not implemented");
                    return NULL;
                case 4: return string_print_float_4;
                case 8: return string_print_float_8;
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
                case 2: return context->output.endianness == BO_ENDIAN_LITTLE ? binary_print_2_le : binary_print_2_be;
                case 4: return context->output.endianness == BO_ENDIAN_LITTLE ? binary_print_4_le : binary_print_4_be;
                case 8: return context->output.endianness == BO_ENDIAN_LITTLE ? binary_print_8_le : binary_print_8_be;
                case 16: return context->output.endianness == BO_ENDIAN_LITTLE ? binary_print_16_le : binary_print_16_be;
                default:
                    bo_notify_error(context, "%d: invalid data width", context->output.data_width);
                    return NULL;
            }
        case TYPE_STRING:
            bo_notify_error(context, "\"String\" is not a valid output format");
        case TYPE_NONE:
            bo_notify_error(context, "Must set output data type before passing data");
            return NULL;
        default:
            bo_notify_error(context, "%d: Unknown data type", context->output.data_type);
            return NULL;
    }
}



// -------
// Buffers
// -------

bo_buffer bo_new_buffer(int size, int overhead)
{
    uint8_t* memory = malloc(size + overhead);
    bo_buffer buffer =
    {
        .start = memory,
        .pos = memory,
        .end = memory + size + overhead,
        .high_water = memory + size,
    };
    return buffer;
}

void bo_free_buffer(bo_buffer* buffer)
{
    if(buffer->start != NULL)
    {
        free(buffer->start);
        buffer->start = buffer->pos = buffer->end = NULL;
    }
}

static bool buffer_is_high_water(bo_buffer* buffer)
{
    return buffer->pos >= buffer->high_water;
}

static bool buffer_is_initialized(bo_buffer* buffer)
{
    return buffer->start != NULL;
}

static bool buffer_is_empty(bo_buffer* buffer)
{
    return buffer->pos == buffer->start;
}

static int buffer_get_used(bo_buffer* buffer)
{
    return buffer->pos - buffer->start;
}

static int buffer_get_remaining(bo_buffer* buffer)
{
    return buffer->end - buffer->pos;
}

static void buffer_clear(bo_buffer* buffer)
{
    buffer->pos = buffer->start;
}

static void buffer_use_space(bo_buffer* buffer, int bytes)
{
    buffer->pos += bytes;
}



// --------
// Internal
// --------

static bool context_user_data_is_owned_by_us(bo_context* context)
{
    return false;
    // return context->on_output == file_on_output;
}

static inline bool can_input_numbers(bo_context* context)
{
    return context->input.data_type != TYPE_NONE;
}

static int trim_length_to_object_boundary(int length, int object_size_in_bytes)
{
    return length - (length % object_size_in_bytes);
}

static inline void swap_endianness(uint8_t* dst, int length)
{
    const int half_length = length / 2;
    for(int i = 0; i < half_length; i++)
    {
        const int ch = dst[i];
        dst[i] = dst[length - i - 1];
        dst[length - i - 1] = ch;
    }
}

static int pad_to_width(uint8_t* buffer, int length, int data_width)
{
    const int dangling_length = (data_width - length) % data_width;
    if(dangling_length > 0)
    {
        memset(buffer+length, 0, dangling_length);
    }
    return dangling_length;
}

// Note: This will run off the end of length to fill out to a multiple of data_width!
static void swap_buffer_endianness(uint8_t* buffer, int length, int data_width)
{
    pad_to_width(buffer, length, data_width);
    const uint8_t* end = buffer + length;
    for(uint8_t* current = buffer; current < end; current += data_width)
    {
        swap_endianness(current, data_width);
    }
}

static void clear_error_condition(bo_context* context)
{
    context->is_error_condition = false;
}


// ---------------
// Buffer Flushing
// ---------------

static void flush_output_buffer(bo_context* context)
{
    if(!context->on_output(context->user_data, context->output_buffer.start, buffer_get_used(&context->output_buffer)))
    {
        mark_error_condition(context);
    }
    buffer_clear(&context->output_buffer);
}

static void flush_work_buffer_binary(bo_context* context)
{
    if(!context->on_output(context->user_data, context->work_buffer.start, buffer_get_used(&context->work_buffer)))
    {
        mark_error_condition(context);
    }
    buffer_clear(&context->work_buffer);
}

static void flush_work_buffer(bo_context* context, bool is_complete_flush)
{
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
    if(!should_continue_parsing(context))
    {
        return;
    }

    int bytes_per_entry = context->output.data_width;
    int work_length = buffer_get_used(&context->work_buffer);
    if(is_complete_flush)
    {
        // Ensure that the partial read at the end gets zeroes instead of random garbage.
        // WORK_BUFFER_OVERHEAD_SIZE makes sure this call doesn't run off the end of the buffer.
        memset(context->work_buffer.pos, 0, 16);
    }
    else
    {
        work_length = trim_length_to_object_boundary(work_length, bytes_per_entry);
    }

    buffer_clear(&context->output_buffer);
    bo_buffer* out = &context->output_buffer;
    uint8_t* end = context->work_buffer.start + work_length;

    for(uint8_t* src = context->work_buffer.start; src < end; src += bytes_per_entry)
    {
        if(context->output.prefix != NULL && *context->output.prefix != 0)
        {
            strcpy(out->pos, context->output.prefix);
            out->pos += strlen(out->pos);
        }

        int bytes_written = string_print(src, out->pos, context->output.text_width, context->output.endianness);
        if(bytes_written < 0)
        {
            bo_notify_error(context, "Error writing string value");
            return;
        }
        out->pos += bytes_written;

        if(src + bytes_per_entry < end && context->output.suffix != NULL)
        {
            strcpy(out->pos, context->output.suffix);
            out->pos += strlen(out->pos);
        }

        if(buffer_is_high_water(out))
        {
            flush_output_buffer(context);
            if(!should_continue_parsing(context))
            {
                return;
            }
        }
    }
}



// ------------------
// Binary Data Adders
// ------------------

static void add_bytes(bo_context* context, uint8_t* ptr, int length)
{
    int remaining = buffer_get_remaining(&context->work_buffer);
    if(length > remaining)
    {
	    memcpy(context->work_buffer.pos, ptr, remaining);
	    buffer_use_space(&context->work_buffer, remaining);
	    flush_work_buffer(context, false);
        if(!should_continue_parsing(context))
	    {
	    	return;
	    }
	    add_bytes(context, ptr + remaining, length - remaining);
        return;
    }

    memcpy(context->work_buffer.pos, ptr, length);
    buffer_use_space(&context->work_buffer, length);
    if(buffer_is_high_water(&context->work_buffer))
    {
        flush_work_buffer(context, false);
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



// ------------
// Internal API
// ------------

bool bo_process_stream_as_binary(bo_context* context, FILE* input_stream)
{
    uint8_t buffer[WORK_BUFFER_SIZE / 2];
    const size_t bytes_to_read = sizeof(buffer);
    size_t bytes_read;
    do
    {
        bytes_read = fread(buffer, 1, bytes_to_read, input_stream);
        if(bytes_read != bytes_to_read)
        {
            if(ferror(input_stream))
            {
                bo_notify_posix_error(context, "Error reading file");
                return false;
            }
        }
        if(context->input.data_width != 1 && context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
        {
            swap_buffer_endianness(buffer, bytes_read, context->input.data_width);
        }
        add_bytes(context, buffer, bytes_read);
        if(!should_continue_parsing(context))
        {
            return false;
        }
    } while(bytes_read == bytes_to_read);

    return true;
}

char* bo_unescape_string(char* str)
{
    char* write_pos = str;
    char* read_pos = str;
    const char* const end_pos = str + strlen(str);
    while(*read_pos != 0)
    {
        char ch = *read_pos++;
        if(ch == '\\')
        {
            char* checkpoint = read_pos - 1;
            ch = *read_pos++;
            switch(ch)
            {
                case 0:
                    return checkpoint;
                case 'r': *write_pos++ = '\r'; break;
                case 'n': *write_pos++ = '\n'; break;
                case 't': *write_pos++ = '\t'; break;
                case '\\': *write_pos++ = '\\'; break;
                case '\"': *write_pos++ = '\"'; break;
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7':
                case '8': case '9': case 'a': case 'b':
                case 'c': case 'd': case 'e': case 'f':
                {
                    char oldch = read_pos[2];
                    read_pos[2] = 0;
                    unsigned int decoded = strtoul(read_pos, NULL, 16);
                    read_pos[2] = oldch;
                    read_pos += 2;
                    *write_pos++ = decoded;
                    break;
                }
                case 'u':
                {
                    if(read_pos + 4 > end_pos)
                    {
                        return checkpoint;
                    }
                    char oldch = read_pos[4];
                    read_pos[4] = 0;
                    unsigned int codepoint = strtoul(read_pos, NULL, 16);
                    read_pos[4] = oldch;
                    read_pos += 4;
                    if(codepoint <= 0x7f)
                    {
                        *write_pos++ = (char)codepoint;
                        break;
                    }
                    if(codepoint <= 0x7ff)
                    {
                        *write_pos++ = (char)((codepoint >> 6) | 0xc0);
                        *write_pos++ = (char)((codepoint & 0x3f) | 0x80);
                        break;
                    }
                    *write_pos++ = (char)((codepoint >> 12) | 0xe0);
                    *write_pos++ = (char)(((codepoint >> 6) & 0x3f) | 0x80);
                    *write_pos++ = (char)((codepoint & 0x3f) | 0x80);
                    break;
                }
                default:
                    return checkpoint;
            }
        }
        else
        {
            *write_pos++ = ch;
        }
    }
    *write_pos = 0;
    return write_pos;
}

// ----------------
// Parser Callbacks
// ----------------

void bo_on_bytes(bo_context* context, char* data, int length)
{
    if(context->input.data_width != 1 && context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
    {
        swap_buffer_endianness(data, length, context->input.data_width);
    }
    add_bytes(context, data, length);    
}

void bo_on_string(bo_context* context, const char* string)
{
    LOG("On string [%s]", string);
    add_bytes(context, (uint8_t*)string, strlen(string));
}

void bo_on_number(bo_context* context, const char* string_value)
{
    LOG("On number [%s]", string_value);
    if(!can_input_numbers(context))
    {
        bo_notify_error(context, "Must set input type before adding numbers");
        return;
    }

    switch(context->input.data_type)
    {
        case TYPE_FLOAT:
            add_float(context, strtod(string_value, NULL));
            return;
        case TYPE_DECIMAL:
            bo_notify_error(context, "TODO: Unimplemented decimal type: %s", string_value);
            return;
        case TYPE_INT:
            add_int(context, strtoul(string_value, NULL, 10));
            return;
        case TYPE_HEX:
            add_int(context, strtoul(string_value, NULL, 16));
            return;
        case TYPE_OCTAL:
            add_int(context, strtoul(string_value, NULL, 8));
            return;
        case TYPE_BOOLEAN:
            add_int(context, strtoul(string_value, NULL, 2));
            return;
        default:
            bo_notify_error(context, "Unknown type %d for value [%s]", context->input.data_type, string_value);
            return;
    }
}

void bo_set_input_type(bo_context* context, const char* string_value)
{
    LOG("Set input type [%s]", string_value);
    context->input.data_type = *string_value++;
    context->input.data_width = strtol(string_value, NULL, 10);
    string_value += context->input.data_width >= 10 ? 2 : 1;
    context->input.endianness = *string_value;
}

void bo_set_input_type_binary(bo_context* context, const char* string_value)
{
    LOG("Set input type binary [%s]", string_value);
    context->input.data_type = TYPE_BINARY;
    context->input.data_width = strtol(string_value, NULL, 10);
    string_value += context->input.data_width >= 10 ? 2 : 1;
    context->input.endianness = *string_value;
}

void bo_set_output_type(bo_context* context, const char* string_value)
{
    LOG("Set output type [%s]", string_value);
    // If the output format changes, everything up to that point must be flushed.
    flush_work_buffer(context, true);
    if(!should_continue_parsing(context))
    {
        return;
    }
    context->output.data_type = *string_value++;
    context->output.data_width = strtol(string_value, NULL, 10);
    string_value += context->output.data_width >= 10 ? 2 : 1;
    context->output.endianness = *string_value++;
    context->output.text_width = strtol(string_value, NULL, 10);
}

void bo_set_output_type_binary(bo_context* context, const char* string_value)
{
    LOG("Set output type binary [%s]", string_value);
    context->output.data_type = TYPE_BINARY;
    context->input.data_width = strtol(string_value, NULL, 10);
    string_value += context->input.data_width >= 10 ? 2 : 1;
    context->input.endianness = *string_value;
    context->output.text_width = 0;
    bo_set_prefix(context, "");
    bo_set_suffix(context, "");
}

void bo_set_prefix(bo_context* context, const char* string_value)
{
    LOG("Set prefix [%s]", string_value);
	if(context->output.prefix != NULL)
	{
		free((void*)context->output.prefix);
	}
	context->output.prefix = strdup(string_value);
}

void bo_set_suffix(bo_context* context, const char* string_value)
{
    LOG("Set suffix [%s]", string_value);
	if(context->output.suffix != NULL)
	{
		free((void*)context->output.suffix);
	}
	context->output.suffix = strdup(string_value);
}

void bo_set_prefix_suffix(bo_context* context, const char* string_value)
{
    LOG("Set prefix-suffix [%s]", string_value);
    switch(*string_value)
    {
        case 's':
            bo_set_suffix(context, " ");
            break;
        case 'c':
            bo_set_suffix(context, ", ");
            switch(context->output.data_type)
            {
                case TYPE_HEX:
                    bo_set_prefix(context, "0x");
                    break;
                case TYPE_OCTAL:
                    bo_set_prefix(context, "0");
                    break;
            }
            break;
        default:
            bo_notify_error(context, "%s: Unknown prefix-suffix preset", string_value);
            return;
    }
}


void bo_on_preset(bo_context* context, const char* string_value)
{
    if(*string_value == 0)
    {
        bo_notify_error(context, "Missing preset value");
        return;
    }
    bo_set_prefix_suffix(context, string_value);
}

void bo_on_prefix(bo_context* context, const char* prefix)
{
    bo_set_prefix(context, prefix);
}

void bo_on_suffix(bo_context* context, const char* suffix)
{
    bo_set_suffix(context, suffix);
}

void bo_on_input_type(bo_context* context, bo_data_type data_type, int data_width, bo_endianness endianness)
{
    LOG("Set input type %c, width %d, endianness %c", data_type, data_width, endianness);
    context->input.data_type = data_type;
    context->input.data_width = data_width;
    context->input.endianness = endianness;
}

void bo_on_output_type(bo_context* context, bo_data_type data_type, int data_width, bo_endianness endianness, int print_width)
{
    LOG("Set input type %c, width %d, endianness %c, print width %d", data_type, data_width, endianness, print_width);
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
        .work_buffer = bo_new_buffer(WORK_BUFFER_SIZE, WORK_BUFFER_OVERHEAD_SIZE),
        .output_buffer = bo_new_buffer(WORK_BUFFER_SIZE * 10, OUTPUT_BUFFER_OVERHEAD_SIZE),
        .input =
        {
            .data_type = TYPE_NONE,
            .data_width = 0,
        },
        .output =
        {
            .data_type = TYPE_NONE,
            .data_width = 0,
            .text_width = 0,
            .prefix = NULL,
            .suffix = NULL,
            .endianness = BO_ENDIAN_LITTLE,
        },
        .on_error = on_error,
        .on_output = on_output,
        .user_data = user_data,
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
    bo_free_buffer(&context->work_buffer);
    bo_free_buffer(&context->output_buffer);
    if(context_user_data_is_owned_by_us(context))
    {
        free(context->user_data);
    }
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
