#include "bo_internal.h"
#include "library_version.h"
#include <stdio.h>
#include <memory.h>

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
static int string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH (uint8_t* src, char* dst, int text_width) \
{ \
	return sprintf(dst, "%0*" #FORMAT, text_width, ((safe_ ## REAL_TYPE ## _ ## DATA_WIDTH *)src)->contents); \
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
static int string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH (uint8_t* src, char* dst, int text_width) \
{ \
	return sprintf(dst, "%.*" #FORMAT, text_width, (double)((safe_ ## REAL_TYPE ## _ ## DATA_WIDTH *)src)->contents); \
}
// TODO: float-2
DEFINE_FLOAT_STRING_PRINTER(float, float, 4, f)
DEFINE_FLOAT_STRING_PRINTER(float, float, 8, f)
// TODO: float-16
// TODO: decimal-8
// TODO: decimal-16

typedef int (*string_printer)(uint8_t* src, char* dst, int text_width);

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
	    			context->on_error("TODO: INT 16 not implemented");
	    			return NULL;
	    		default:
	    			context->on_error("%d: invalid data width", context->output.data_width);
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
	    			context->on_error("TODO: HEX 16 not implemented");
	    			return NULL;
	    		default:
	    			context->on_error("%d: invalid data width", context->output.data_width);
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
	    			context->on_error("TODO: OCTAL 16 not implemented");
	    			return NULL;
	    		default:
	    			context->on_error("%d: invalid data width", context->output.data_width);
	    			return NULL;
	    	}
	    }
    	case TYPE_BOOLEAN:
			context->on_error("TODO: BOOLEAN not implemented");
    		return NULL;
    	case TYPE_FLOAT:
    	{
	    	switch(context->output.data_width)
	    	{
	    		case 2:
	    			context->on_error("TODO: FLOAT 2not implemented");
	    			return NULL;
	    		case 4: return string_print_float_4;
	    		case 8: return string_print_float_8;
	    		case 16:
	    			context->on_error("TODO: FLOAT 16 not implemented");
	    			return NULL;
	    		default:
	    			context->on_error("%d: invalid data width", context->output.data_width);
	    			return NULL;
	    	}
	    }
    	case TYPE_DECIMAL:
			context->on_error("TODO: DECIMAL not implemented");
    		return NULL;
    	case TYPE_STRING:
			context->on_error("\"String\" is not a valid output format");
		default:
			context->on_error("%d: Unknown data type", context->output.data_type);
			return NULL;
	}
}

static int output(bo_context* context, uint8_t* src, int src_length, uint8_t* dst, int dst_length)
{
	if(src_length > dst_length)
	{
		context->on_error("Not enough room in destination buffer");
		return -1;
	}

	if(context->output.data_type == TYPE_BINARY)
	{
		memcpy(dst, src, src_length);
		return src_length;
	}

	char buffer[100];
	if(context->output.prefix != NULL)
	{
		strcpy(buffer, context->output.prefix);
	}
	else
	{
		buffer[0] = 0;
	}
	char* const buffer_start = buffer + strlen(buffer);

	const uint8_t* const dst_end = dst + dst_length;
	int bytes_per_entry = context->output.data_width;
	string_printer string_print = get_string_printer(context);
	if(string_print == NULL)
	{
		// Assume string_print() reported the error.
		return -1;
	}

	uint8_t* dst_pos = dst;
    for(int i = 0; i < src_length; i += bytes_per_entry)
    {
    	uint8_t overflow_buffer[17];
    	uint8_t* src_ptr = src + i;
    	if(src_length - i < bytes_per_entry)
    	{
    		memset(overflow_buffer, 0, sizeof(overflow_buffer));
    		memcpy(overflow_buffer, src_ptr, src_length - i);
    		src_ptr = overflow_buffer;
    	}

    	int bytes_written = string_print(src_ptr, buffer_start, context->output.text_width);
    	if(bytes_written < 0)
    	{
    		context->on_error("Error writing string value");
    		return -1;
    	}

    	char* buffer_pos = buffer_start + bytes_written;
        if(i < src_length - bytes_per_entry && context->output.suffix != NULL)
        {
	    	strcpy(buffer_pos, context->output.suffix);
	    	buffer_pos += strlen(buffer_pos);
	    }

	    int entry_length = buffer_pos - buffer;
	    if(dst_pos + entry_length >= dst_end)
	    {
			context->on_error("Not enough room in destination buffer");
	    	return -1;
	    }

	    memcpy(dst_pos, buffer, entry_length + 1);
        dst_pos += entry_length;
    }
    return (uint8_t*)dst_pos - dst;
}

static bool flush_buffer(bo_context* context)
{
	if(context->work_buffer.start == NULL)
	{
		return true;
	}

	int work_filled = context->work_buffer.pos - context->work_buffer.start;
	int output_remaining = context->output_buffer.end - context->output_buffer.pos;
	int used_bytes = output(context, context->work_buffer.start, work_filled, context->output_buffer.pos, output_remaining);

	if(used_bytes < 0)
    {
		// Assume output() reported the error.
    	return false;
    }
    context->output_buffer.pos += used_bytes;
	context->work_buffer.pos = context->work_buffer.start;
    return true;
}

static bool add_bytes(bo_context* context, uint8_t* ptr, int length)
{
    int remaining = context->work_buffer.end - context->work_buffer.pos;
    if(length > remaining)
    {
	    memcpy(context->work_buffer.pos, ptr, remaining);
	    context->work_buffer.pos += remaining;
	    if(!flush_buffer(context))
	    {
			// Assume flush_buffer() reported the error.
	    	return false;
	    }
	    return add_bytes(context, ptr + remaining, length - remaining);
    }

    memcpy(context->work_buffer.pos, ptr, length);
    context->work_buffer.pos += length;
    if(remaining - length < 16)
    {
        return flush_buffer(context);
    }
    return true;
}

static bool add_int(bo_context* context, int64_t src_value)
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
            return add_bytes(context, (uint8_t*)&value, sizeof(value));
        }
        case WIDTH_4:
        {
            uint32_t value = (uint32_t)src_value;
            return add_bytes(context, (uint8_t*)&value, sizeof(value));
        }
        case WIDTH_8:
        {
            uint64_t value = (uint64_t)src_value;
            return add_bytes(context, (uint8_t*)&value, sizeof(value));
        }
        case WIDTH_16:
		    context->on_error("TODO: Width 16 not implemented yet");
		    return false;
        default:
		    context->on_error("Unhandled type/width");
		    return false;
    }
}

static bool add_float(bo_context* context, double src_value)
{
    switch(context->input.data_width)
    {
        case WIDTH_4:
        {
            float value = (float)src_value;
            return add_bytes(context, (uint8_t*)&value, sizeof(value));
        }
        case WIDTH_8:
        {
            double value = (double)src_value;
            return add_bytes(context, (uint8_t*)&value, sizeof(value));
        }
        case WIDTH_16:
		    context->on_error("TODO: Width 16 not implemented yet");
		    return false;
        default:
		    context->on_error("Unhandled type/width");
		    return false;
    }
}

const char* bo_version()
{
    return BO_VERSION;
}

bo_buffer bo_new_buffer(int size)
{
    uint8_t* memory = malloc(size);
    bo_buffer buffer =
    {
        .start = memory,
        .pos = memory,
        .end = memory + size,
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
    switch(context->input.data_type)
    {
        case TYPE_FLOAT:
            return add_float(context, strtod(string_value, NULL));
        case TYPE_DECIMAL:
            context->on_error("TODO: Unimplemented decimal type: %s", string_value);
            return true;
        case TYPE_INT:
            return add_int(context, strtoll(string_value, NULL, 10));
        case TYPE_HEX:
            return add_int(context, strtoll(string_value, NULL, 16));
        case TYPE_OCTAL:
            return add_int(context, strtoll(string_value, NULL, 8));
        case TYPE_BOOLEAN:
            return add_int(context, strtoll(string_value, NULL, 2));
        default:
            context->on_error("Unknown type %d for value [%s]", context->input.data_type, string_value);
        	return false;
    }
}

bool bo_finish(bo_context* context)
{
    bool success = flush_buffer(context);
    bo_free_buffer(&context->work_buffer);
	if(context->output.prefix != NULL)
	{
		free((void*)context->output.prefix);
	}
	if(context->output.suffix != NULL)
	{
		free((void*)context->output.suffix);
	}
	return success;
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
    context->output.data_type = *string_value++;
    context->output.data_width = strtol(string_value, NULL, 10);
    string_value += context->output.data_width >= 10 ? 2 : 1;
    context->output.endianness = *string_value++;
    context->output.text_width = strtol(string_value, NULL, 10);
	return true;
}

bool bo_set_output_binary(bo_context* context)
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

bo_context bo_new_context(int work_buffer_size, uint8_t* output, int output_length, error_callback on_error)
{
	bo_context context =
	{
        .work_buffer = bo_new_buffer(work_buffer_size),
        .output_buffer =
        {
        	.start = output,
        	.pos = output,
        	.end = output + output_length,
        },
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
	};
	return context;
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
