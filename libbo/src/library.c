#include "bo_internal.h"
#include "library_version.h"
#include <stdio.h>
#include <memory.h>
#include <stdarg.h>
#include <errno.h>


// An overhead size of 32 ensures that for object sizes up to 128 bits,
// there's always room for 128 bits of zero filling at the end.
#define WORK_BUFFER_OVERHEAD_SIZE 32

// Use an overhead value large enough that the string printers won't blast past it in a single write.
#define OUTPUT_BUFFER_OVERHEAD_SIZE 100

void bo_notify_error(bo_context* context, char* fmt, ...)
{
    char buffer[500];

    va_list args;
    va_start(args, fmt);
    snprintf(buffer, sizeof(buffer), fmt, args);
    buffer[sizeof(buffer)-1] = 0;
    va_end(args);

    context->on_error(context->user_data, buffer);
}

static void bo_notify_posix_error(bo_context* context, char* fmt, ...)
{
    int error_num = errno;
    char buffer[500];

    va_list args;
    va_start(args, fmt);
    snprintf(buffer, sizeof(buffer), fmt, args);
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

    context->on_error(context->user_data, buffer);
}

static bool is_high_water(bo_buffer* buffer)
{
    return buffer->pos >= buffer->high_water;
}

static void buffer_clear(bo_buffer* buffer)
{
    buffer->pos = buffer->start;
}

static void byte_swap(uint8_t* dst, uint8_t* src, int length)
{
	for(int i = 0; i < length; i++)
	{
		dst[i] = src[length - i - 1];
	}
}

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
	byte_swap(buffer, src, DATA_WIDTH); \
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
	byte_swap(buffer, src, DATA_WIDTH); \
	return string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH ## _internal (buffer, dst, text_width); \
}
// TODO: float-2
DEFINE_FLOAT_STRING_PRINTER(float, float, 4, f)
DEFINE_FLOAT_STRING_PRINTER(float, float, 8, f)
// TODO: float-16
// TODO: decimal-8
// TODO: decimal-16

typedef int (*string_printer)(uint8_t* src, char* dst, int text_width, bo_endianness endianness);

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
    	case TYPE_STRING:
			bo_notify_error(context, "\"String\" is not a valid output format");
    	case TYPE_NONE:
			bo_notify_error(context, "You must set an output data type before passing data");
			return NULL;
		default:
			bo_notify_error(context, "%d: Unknown data type", context->output.data_type);
			return NULL;
	}
}

static inline bool can_input_numbers(bo_context* context)
{
    return context->input.data_type != TYPE_NONE;
}

static void clear_output_buffer(bo_context* context)
{
    buffer_clear(&context->output_buffer);
}

static bool flush_output_buffer(bo_context* context)
{
    bool result = context->on_output(context->user_data, context->output_buffer.start, context->output_buffer.pos - context->output_buffer.start);
    clear_output_buffer(context);
    return result;
}

static bool flush_work_buffer_binary(bo_context* context)
{
    bool result = context->on_output(context->user_data, context->work_buffer.start, context->work_buffer.pos - context->work_buffer.start);
    buffer_clear(&context->work_buffer);
    return result;
}

static int trim_length_to_object_boundary(int length, int object_size_in_bytes)
{
    return length - (length % object_size_in_bytes);
}

static bool flush_work_buffer(bo_context* context, bool is_complete_flush)
{
    if(context->work_buffer.start == NULL || context->work_buffer.pos == context->work_buffer.start)
    {
        return true;
    }

    if(context->output.data_type == TYPE_BINARY)
    {
        return flush_work_buffer_binary(context);
    }

    string_printer string_print = get_string_printer(context);
    if(string_print == NULL)
    {
        // Assume string_print() reported the error.
        return false;
    }

    int bytes_per_entry = context->output.data_width;
    int work_length = context->work_buffer.pos - context->work_buffer.start;
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

    clear_output_buffer(context);
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
            return -1;
        }
        out->pos += bytes_written;

        if(src + bytes_per_entry < end && context->output.suffix != NULL)
        {
            strcpy(out->pos, context->output.suffix);
            out->pos += strlen(out->pos);
        }

        if(is_high_water(out))
        {
            if(!flush_output_buffer(context))
            {
                return false;
            }
        }
    }
    return true;
}

// static int output(bo_context* context, uint8_t* src, int src_length, uint8_t* dst, int dst_length)
// {
// 	if(src_length > dst_length)
// 	{
// 		bo_notify_error(context, "Not enough room in destination buffer");
// 		return -1;
// 	}

// 	if(context->output.data_type == TYPE_BINARY)
// 	{
// 		memcpy(dst, src, src_length);
// 		return src_length;
// 	}

// 	char buffer[100];
// 	if(context->output.prefix != NULL)
// 	{
// 		strcpy(buffer, context->output.prefix);
// 	}
// 	else
// 	{
// 		buffer[0] = 0;
// 	}
// 	char* const buffer_start = buffer + strlen(buffer);

// 	const uint8_t* const dst_end = dst + dst_length;
// 	int bytes_per_entry = context->output.data_width;
// 	string_printer string_print = get_string_printer(context);
// 	if(string_print == NULL)
// 	{
// 		// Assume string_print() reported the error.
// 		return -1;
// 	}

// 	uint8_t* dst_pos = dst;
//     for(int i = 0; i < src_length; i += bytes_per_entry)
//     {
//     	uint8_t overflow_buffer[17];
//     	uint8_t* src_ptr = src + i;
//     	if(src_length - i < bytes_per_entry)
//     	{
//     		memset(overflow_buffer, 0, sizeof(overflow_buffer));
//     		memcpy(overflow_buffer, src_ptr, src_length - i);
//     		src_ptr = overflow_buffer;
//     	}

//     	int bytes_written = string_print(src_ptr, buffer_start, context->output.text_width, context->output.endianness);
//     	if(bytes_written < 0)
//     	{
//     		bo_notify_error(context, "Error writing string value");
//     		return -1;
//     	}

//     	char* buffer_pos = buffer_start + bytes_written;
//         if(i < src_length - bytes_per_entry && context->output.suffix != NULL)
//         {
// 	    	strcpy(buffer_pos, context->output.suffix);
// 	    	buffer_pos += strlen(buffer_pos);
// 	    }

// 	    int entry_length = buffer_pos - buffer;
// 	    if(dst_pos + entry_length >= dst_end)
// 	    {
// 			bo_notify_error(context, "Not enough room in destination buffer");
// 	    	return -1;
// 	    }

// 	    memcpy(dst_pos, buffer, entry_length + 1);
//         dst_pos += entry_length;
//     }
//     return (uint8_t*)dst_pos - dst;
// }

// static bool flush_buffer(bo_context* context)
// {
//     // TODO: Flush to a data width
// 	if(context->work_buffer.start == NULL || context->work_buffer.pos == context->work_buffer.start)
// 	{
// 		return true;
// 	}

// 	int work_filled = context->work_buffer.pos - context->work_buffer.start;
// 	int output_remaining = context->output_buffer.end - context->output_buffer.pos;
// 	int used_bytes = output(context, context->work_buffer.start, work_filled, context->output_buffer.pos, output_remaining);

// 	if(used_bytes < 0)
//     {
// 		// Assume output() reported the error.
//     	return false;
//     }
//     context->output_buffer.pos += used_bytes;
// 	context->work_buffer.pos = context->work_buffer.start;

//     if(context->output_stream != NULL)
//     {
//         int bytes_to_write = context->output_buffer.pos - context->output_buffer.start;
//         int bytes_written = fwrite(context->output_buffer.start, 1, bytes_to_write, context->output_stream);
//         context->output_buffer.pos = context->output_buffer.start;
//         if(bytes_written != bytes_to_write)
//         {
//             perror("Error writing to output stream");
//             return false;
//         }
//     }

//     return true;
// }

static bool add_bytes(bo_context* context, uint8_t* ptr, int length)
{
    int remaining = context->work_buffer.end - context->work_buffer.pos;
    if(length > remaining)
    {
	    memcpy(context->work_buffer.pos, ptr, remaining);
	    context->work_buffer.pos += remaining;
	    if(!flush_work_buffer(context, false))
	    {
			// Assume flush_work_buffer() reported the error.
	    	return false;
	    }
	    return add_bytes(context, ptr + remaining, length - remaining);
    }

    memcpy(context->work_buffer.pos, ptr, length);
    context->work_buffer.pos += length;
    if(is_high_water(&context->work_buffer))
    {
        return flush_work_buffer(context, false);
    }
    return true;
}

static bool add_int(bo_context* context, uint64_t src_value)
{
    switch(context->input.data_width)
    {
        case WIDTH_1:
        {
            uint8_t value = (uint8_t)src_value;
            return add_bytes(context, (uint8_t*)&value, sizeof(value));
        }
        case WIDTH_2:
        {
            uint16_t value = (uint16_t)src_value;
            if(context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
            {
            	uint8_t buff[sizeof(value)];
            	byte_swap(buff, (uint8_t*)&value, sizeof(value));
	            return add_bytes(context, buff, sizeof(value));
            }
            return add_bytes(context, (uint8_t*)&value, sizeof(value));
        }
        case WIDTH_4:
        {
            uint32_t value = (uint32_t)src_value;
            if(context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
            {
            	uint8_t buff[sizeof(value)];
            	byte_swap(buff, (uint8_t*)&value, sizeof(value));
	            return add_bytes(context, buff, sizeof(value));
            }
            return add_bytes(context, (uint8_t*)&value, sizeof(value));
        }
        case WIDTH_8:
        {
            uint64_t value = (uint64_t)src_value;
            if(context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
            {
            	uint8_t buff[sizeof(value)];
            	byte_swap(buff, (uint8_t*)&value, sizeof(value));
	            return add_bytes(context, buff, sizeof(value));
            }
            return add_bytes(context, (uint8_t*)&value, sizeof(value));
        }
        case WIDTH_16:
		    bo_notify_error(context, "TODO: Int width 16 not implemented yet");
		    return false;
        default:
		    bo_notify_error(context, "$d: Invalid int width", context->input.data_width);
		    return false;
    }
}

static bool add_float(bo_context* context, double src_value)
{
    switch(context->input.data_width)
    {
        case WIDTH_2:
		    bo_notify_error(context, "TODO: Float width 2 not implemented yet");
		    return false;
        case WIDTH_4:
        {
            float value = (float)src_value;
            if(context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
            {
            	uint8_t buff[sizeof(value)];
            	byte_swap(buff, (uint8_t*)&value, sizeof(value));
	            return add_bytes(context, buff, sizeof(value));
            }
            return add_bytes(context, (uint8_t*)&value, sizeof(value));
        }
        case WIDTH_8:
        {
            double value = (double)src_value;
            if(context->input.endianness != BO_NATIVE_INT_ENDIANNESS)
            {
            	uint8_t buff[sizeof(value)];
            	byte_swap(buff, (uint8_t*)&value, sizeof(value));
	            return add_bytes(context, buff, sizeof(value));
            }
            return add_bytes(context, (uint8_t*)&value, sizeof(value));
        }
        case WIDTH_16:
		    bo_notify_error(context, "TODO: Float width 16 not implemented yet");
		    return false;
        default:
		    bo_notify_error(context, "%d: Invalid float width", context->input.data_width);
		    return false;
    }
}

const char* bo_version()
{
    return BO_VERSION;
}

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

bool bo_on_string(bo_context* context, const char* string)
{
    return add_bytes(context, (uint8_t*)string, strlen(string));
}

bool bo_on_number(bo_context* context, const char* string_value)
{
    if(!can_input_numbers(context))
    {
        bo_notify_error(context, "Must set input type before adding numbers");
        return false;
    }

    switch(context->input.data_type)
    {
        case TYPE_FLOAT:
            return add_float(context, strtod(string_value, NULL));
        case TYPE_DECIMAL:
            bo_notify_error(context, "TODO: Unimplemented decimal type: %s", string_value);
            return true;
        case TYPE_INT:
            return add_int(context, strtoul(string_value, NULL, 10));
        case TYPE_HEX:
            return add_int(context, strtoul(string_value, NULL, 16));
        case TYPE_OCTAL:
            return add_int(context, strtoul(string_value, NULL, 8));
        case TYPE_BOOLEAN:
            return add_int(context, strtoul(string_value, NULL, 2));
        default:
            bo_notify_error(context, "Unknown type %d for value [%s]", context->input.data_type, string_value);
        	return false;
    }
}

bool bo_set_input_type(bo_context* context, const char* string_value)
{
    context->input.data_type = *string_value++;
    context->input.data_width = strtol(string_value, NULL, 10);
    string_value += context->input.data_width >= 10 ? 2 : 1;
    context->input.endianness = *string_value;
    return true;
}

bool bo_set_output_type(bo_context* context, const char* string_value)
{
    // If the output format changes, everything up to that point must be flushed.
    if(!flush_work_buffer(context, true))
    {
        return false;
    }
    context->output.data_type = *string_value++;
    context->output.data_width = strtol(string_value, NULL, 10);
    string_value += context->output.data_width >= 10 ? 2 : 1;
    context->output.endianness = *string_value++;
    context->output.text_width = strtol(string_value, NULL, 10);
	return true;
}

bool bo_set_input_type_binary(bo_context* context)
{
    context->input.data_type = TYPE_BINARY;
    context->input.data_width = 0;
    context->input.endianness = BO_ENDIAN_LITTLE;
    return true;
}

bool bo_set_output_type_binary(bo_context* context)
{
    context->output.data_type = TYPE_BINARY;
    context->output.data_width = 0;
    context->output.endianness = BO_ENDIAN_LITTLE;
    context->output.text_width = 0;
    bo_set_prefix(context, "");
    bo_set_suffix(context, "");
    return true;
}

bool bo_set_prefix(bo_context* context, const char* string_value)
{
	if(context->output.prefix != NULL)
	{
		free((void*)context->output.prefix);
	}
	context->output.prefix = strdup(string_value);
	return true;
}

bool bo_set_suffix(bo_context* context, const char* string_value)
{
	if(context->output.suffix != NULL)
	{
		free((void*)context->output.suffix);
	}
	context->output.suffix = strdup(string_value);
	return true;
}

bool bo_set_prefix_suffix(bo_context* context, const char* string_value)
{
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
            return false;
    }
    return true;
}

static void* new_context(void* user_data, output_callback on_output, error_callback on_error)
{
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

typedef struct
{
    bo_context* context;
    FILE* output_stream;
    error_callback on_error;
    void* user_data;
} wrapped_user_data;

static bool file_on_output(void* void_user_data, char* data, int length)
{
    wrapped_user_data* wrapped = (wrapped_user_data*)void_user_data;
    int bytes_written = fwrite(data, 1, length, wrapped->output_stream);
    if(bytes_written != length)
    {
        bo_notify_posix_error(wrapped->context, "Error writing to output stream");
        return false;
    }
    return true;
}

static void file_on_error(void* void_user_data, const char* message)
{
    wrapped_user_data* wrapped = (wrapped_user_data*)void_user_data;
    wrapped->on_error(wrapped->user_data, message);
}

static bool context_user_data_is_owned_by_us(bo_context* context)
{
    return context->on_output == file_on_output;
}

void* bo_new_callback_context(void* user_data, output_callback on_output, error_callback on_error)
{
    return new_context(user_data, on_output, on_error);
}

void* bo_new_stream_context(void* user_data, FILE* output_stream, error_callback on_error)
{
    wrapped_user_data* wrapped = malloc(sizeof(wrapped_user_data));
    wrapped->output_stream = output_stream;
    wrapped->on_error = on_error;
    wrapped->user_data = user_data;
    bo_context* context = new_context(wrapped, file_on_output, file_on_error);
    wrapped->context = context;
    return context;
}

bool bo_flush_and_destroy_context(void* void_context)
{
    bo_context* context = (bo_context*)void_context;
    bool is_successful = flush_work_buffer(context, true);
    flush_output_buffer(context);
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
        if(!add_bytes(context, buffer, bytes_read))
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
