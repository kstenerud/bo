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
static void string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH (uint8_t* src, char* dst, int text_width, int precision __attribute__((unused))) \
{ \
	sprintf(dst, "%0*" #FORMAT, text_width, ((safe_ ## REAL_TYPE ## _ ## DATA_WIDTH *)src)->contents); \
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
static void string_print_ ## NAMED_TYPE ## _ ## DATA_WIDTH (uint8_t* src, char* dst, int text_width, int precision) \
{ \
		sprintf(dst, "%0*.*" #FORMAT, text_width, precision, (double)((safe_ ## REAL_TYPE ## _ ## DATA_WIDTH *)src)->contents); \
}
// TODO: float-2
DEFINE_FLOAT_STRING_PRINTER(float, float, 4, f)
DEFINE_FLOAT_STRING_PRINTER(float, float, 8, f)
// TODO: float-16
// TODO: decimal-8
// TODO: decimal-16

typedef void (*string_printer)(uint8_t* src, char* dst, int text_width, int precision);

static string_printer get_string_printer(bo_context* context)
{
	switch(context->config->output.data_type)
	{
	    case TYPE_INT:
	    {
	    	switch(context->config->output.data_width)
	    	{
	    		case 1: return string_print_int_1;
	    		case 2: return string_print_int_2;
	    		case 4: return string_print_int_4;
	    		case 8: return string_print_int_8;
	    		case 16:
	    			// TODO
	    			return NULL;
	    		default:
	    			// TODO: Report error
	    			return NULL;
	    	}
	    }
	    case TYPE_HEX:
	    {
	    	switch(context->config->output.data_width)
	    	{
	    		case 1: return string_print_hex_1;
	    		case 2: return string_print_hex_2;
	    		case 4: return string_print_hex_4;
	    		case 8: return string_print_hex_8;
	    		case 16:
	    			// TODO
	    			return NULL;
	    		default:
	    			// TODO: Report error
	    			return NULL;
	    	}
	    }
    	case TYPE_OCTAL:
	    {
	    	switch(context->config->output.data_width)
	    	{
	    		case 1: return string_print_octal_1;
	    		case 2: return string_print_octal_2;
	    		case 4: return string_print_octal_4;
	    		case 8: return string_print_octal_8;
	    		case 16:
	    			// TODO
	    			return NULL;
	    		default:
	    			// TODO: Report error
	    			return NULL;
	    	}
	    }
    	case TYPE_BOOLEAN:
    		// TODO
    		return NULL;
    	case TYPE_FLOAT:
    	{
	    	switch(context->config->output.data_width)
	    	{
	    		case 2:
	    			// TODO
	    			return NULL;
	    		case 4: return string_print_float_4;
	    		case 8: return string_print_float_8;
	    		case 16:
	    			// TODO
	    			return NULL;
	    		default:
	    			// TODO: Report error
	    			return NULL;
	    	}
	    }
    	case TYPE_DECIMAL:
			// TODO
    		return NULL;
    	case TYPE_STRING:
			// TODO: Report error
		default:
			// TODO: Report error
			return NULL;
	}
}

static int output(bo_context* context, uint8_t* src, int src_length, uint8_t* dst, int dst_length)
{
	char buffer[100];
	strcpy(buffer, context->config->output.prefix);
	char* const buffer_ptr = buffer + strlen(buffer);

	const uint8_t* const dst_end = dst + dst_length;
	int bytes_per_entry = context->config->output.data_width;
	string_printer string_print = get_string_printer(context);
	if(string_print == NULL)
	{
		return -1;
	}

	uint8_t* dst_pos = dst;
    for(int i = 0; i < src_length; i += bytes_per_entry)
    {
    	// TODO: Make sure there's enough data to print!
    	string_print(src+i, buffer_ptr, context->config->output.text_width, context->config->output.precision);
    	char* buffer_pos = buffer_ptr + strlen(buffer_ptr);
        if(i < src_length - bytes_per_entry)
        {
	    	strcpy(buffer_pos, context->config->output.suffix);
	    }
	    int entry_length = strlen(buffer);
	    if(dst_pos + entry_length >= dst_end)
	    {
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

	int work_length = context->work_buffer.pos - context->work_buffer.start;
	int output_length = context->work_buffer.end - context->work_buffer.pos;
	int used_bytes = output(context, context->work_buffer.start, work_length, context->output_buffer.pos, output_length);

	if(used_bytes < 0)
    {
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
    switch(context->data_width)
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
        default:
		    context->config->on_error("Unhandled type/width");
		    return false;
    }
}

static bool add_float(bo_context* context, double src_value)
{
    switch(context->data_width)
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
        default:
		    context->config->on_error("Unhandled type/width");
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
    add_bytes(context, (uint8_t*)string, strlen(string));
    return true;
}

bool bo_on_number(bo_context* context, const char* string_value)
{
    switch(context->data_type)
    {
        case TYPE_FLOAT:
            return add_float(context, strtod(string_value, NULL));
        case TYPE_DECIMAL:
            context->config->on_error("Unhandled type: %s", string_value);
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
        	return false;
    }
}

void bo_finish(bo_context* context)
{
    flush_buffer(context);
    bo_free_buffer(&context->work_buffer);
}

bool bo_set_input_type(bo_context* context, const char* string_value)
{
    context->data_type = *string_value++;
    int width = strtol(string_value, NULL, 10);
    // TODO: Check width
    context->data_width = width;
    return true;
}

bool bo_set_output_type(bo_context* context, const char* string_value)
{
	return true;
}

bool bo_set_prefix(bo_context* context, const char* string_value)
{
	return true;
}

bool bo_set_suffix(bo_context* context, const char* string_value)
{
	return true;
}

bo_context bo_new_context(int work_buffer_size, uint8_t* output, int output_length, bo_config* config)
{
	bo_context context =
	{
        .data_type = TYPE_NONE,
        .work_buffer = bo_new_buffer(work_buffer_size),
        .output_buffer =
        {
        	.start = output,
        	.pos = output,
        	.end = output + output_length,
        },
        .config = config,
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
                    // TODO
                    break;
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
    return NULL;
}
