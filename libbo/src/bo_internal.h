#ifndef bo_internal_H
#define bo_internal_H
#ifdef __cplusplus
extern "C" {
#endif


#include "bo/bo.h"
#include <stdbool.h>
#include <stdint.h>

#define LOG(FMT, ...)
// #define LOG(...) do {fprintf(stdout, "LOG: ");fprintf(stdout, __VA_ARGS__);fprintf(stdout, "\n");fflush(stdout);} while(0)

#define BO_NATIVE_INT_ENDIANNESS (((__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) * BO_ENDIAN_LITTLE) + \
                                  ((__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) * BO_ENDIAN_BIG))
#define BO_NATIVE_FLOAT_ENDIANNESS (((__FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__) * BO_ENDIAN_LITTLE) + \
                                    ((__FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__) * BO_ENDIAN_BIG))

#define EARLY_EXIT_BINARY_MODE_MARKER 0x1234


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
    FILE* output_stream;
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
} bo_context;


bool bo_process_stream_as_binary(bo_context* context, FILE* input_stream);

bool bo_on_string(bo_context* context, const char* string);
bool bo_on_number(bo_context* context, const char* string_value);

bool bo_set_input_type(bo_context* context, const char* string_value);
bool bo_set_output_type(bo_context* context, const char* string_value);
bool bo_set_input_type_binary(bo_context* context, const char* string_value);
bool bo_set_output_type_binary(bo_context* context, const char* string_value);
bool bo_set_prefix(bo_context* context, const char* string_value);
bool bo_set_suffix(bo_context* context, const char* string_value);
bool bo_set_prefix_suffix(bo_context* context, const char* string_value);

/**
 * Escapes a string in-place (modifies the original string).
 *
 * @param str The string to escape (this string may get modified).
 * @return pointer to the offending character on failure, pointer to the end of the string (\0) on success.
 */
char* bo_unescape_string(char* str);

void bo_notify_error(bo_context* context, char* fmt, ...);


#ifdef __cplusplus
}
#endif
#endif // bo_internal_H
